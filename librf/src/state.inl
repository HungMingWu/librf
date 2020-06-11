
namespace resumef
{
	template<_PromiseT Promise>
	void state_future_t::promise_initial_suspend(coroutine_handle<Promise> handler)
	{
		assert(this->_scheduler == nullptr);
		assert(!this->_coro);

		this->_initor = handler;
		this->_is_initor = initor_type::Initial;
	}

	template<_PromiseT Promise>
	void state_future_t::promise_final_suspend(coroutine_handle<Promise> handler)
	{
		std::scoped_lock __guard(this->_mtx);

		this->_initor = handler;
		this->_is_initor = initor_type::Final;

		scheduler_t* sch = this->get_scheduler();
		assert(sch != nullptr);

		if (this->has_handler_skip_lock())
			sch->add_generator(this);
		sch->del_final(this);
	}

	template<_PromiseT Promise>
	void state_future_t::future_await_suspend(coroutine_handle<Promise> handler)
	{
		Promise& promise = handler.promise();
		auto* parent_state = promise.get_state();
		scheduler_t* sch = parent_state->get_scheduler();

		std::scoped_lock __guard(this->_mtx);

		if (this != parent_state)
		{
			this->_parent = parent_state;
			this->_scheduler = sch;
		}

		if (!this->_coro)
			this->_coro = handler;

		if (sch != nullptr && this->is_ready())
			sch->add_generator(this);
	}

	//------------------------------------------------------------------------------------------------

	template<_PromiseT Promise>
	void state_t<void>::promise_yield_value(Promise* promise)
	{
		coroutine_handle<Promise> handler = coroutine_handle<Promise>::from_promise(*promise);

		std::scoped_lock __guard(this->_mtx);

		if (!handler.done())
		{
			if (!this->_coro)
				this->_coro = handler;
		}

		this->_has_value.store(result_type::Value, std::memory_order_release);

		if (!handler.done())
		{
			scheduler_t* sch = this->get_scheduler();
			if (sch != nullptr)
				sch->add_generator(this);
		}
	}

	//------------------------------------------------------------------------------------------------

	template<typename _Ty>
	template<_PromiseT Promise, typename U>
	void state_t<_Ty>::promise_yield_value(Promise* promise, U&& val)
	{
		coroutine_handle<Promise> handler = coroutine_handle<Promise>::from_promise(*promise);

		std::scoped_lock __guard(this->_mtx);

		if (!handler.done())
		{
			if (this->_coro == nullptr)
				this->_coro = handler;
		}

		set_value_internal(std::forward<U>(val));

		if (!handler.done())
		{
			scheduler_t* sch = this->get_scheduler();
			if (sch != nullptr)
				sch->add_generator(this);
		}
	}

	template<typename _Ty>
	auto state_t<_Ty>::future_await_resume() -> value_type
	{
		std::scoped_lock __guard(this->_mtx);

		switch (this->_has_value.load(std::memory_order_acquire))
		{
		case result_type::None:
			std::rethrow_exception(std::make_exception_ptr(future_exception{ error_code::not_ready }));
			break;
		case result_type::Exception:
			std::rethrow_exception(std::exchange(this->_exception, nullptr));
			break;
		default:
			break;
		}

		return std::move(this->_value);
	}

	template<typename _Ty>
	template<typename U>
	void state_t<_Ty>::set_value_internal(U&& val)
	{
		switch (_has_value.load(std::memory_order_acquire))
		{
		case result_type::Value:
			_value = std::forward<U>(val);
			break;
		case result_type::Exception:
			_exception.~exception_ptr();
		default:
			new (&this->_value) value_type(std::forward<U>(val));
			this->_has_value.store(result_type::Value, std::memory_order_release);
			break;
		}
	}

	template<typename _Ty>
	void state_t<_Ty>::set_exception_internal(std::exception_ptr e)
	{
		switch (_has_value.load(std::memory_order_acquire))
		{
		case result_type::Exception:
			_exception = std::move(e);
			break;
		case result_type::Value:
			_value.~value_type();
		default:
			new (&this->_exception) std::exception_ptr(std::move(e));
			this->_has_value.store(result_type::Exception, std::memory_order_release);
			break;
		}
	}

	template<typename _Ty>
	template<typename U>
	void state_t<_Ty>::set_value(U&& val)
	{
		std::scoped_lock __guard(this->_mtx);
		set_value_internal(std::forward<U>(val));

		scheduler_t* sch = this->get_scheduler();
		if (sch != nullptr)
		{
			if (this->has_handler_skip_lock())
				sch->add_generator(this);
			else
				sch->del_final(this);
		}
	}

	template<typename _Ty>
	void state_t<_Ty>::set_exception(std::exception_ptr e)
	{
		std::scoped_lock __guard(this->_mtx);
		set_exception_internal(std::move(e));

		scheduler_t* sch = this->get_scheduler();
		if (sch != nullptr)
		{
			if (this->has_handler_skip_lock())
				sch->add_generator(this);
			else
				sch->del_final(this);
		}
	}

	//------------------------------------------------------------------------------------------------

	template<typename _Ty>
	template<_PromiseT Promise>
	void state_t<_Ty&>::promise_yield_value(Promise* promise, reference_type val)
	{
		coroutine_handle<Promise> handler = coroutine_handle<Promise>::from_promise(*promise);

		std::scoped_lock __guard(this->_mtx);

		if (!handler.done())
		{
			if (this->_coro == nullptr)
				this->_coro = handler;
		}

		set_value_internal(val);

		if (!handler.done())
		{
			scheduler_t* sch = this->get_scheduler();
			if (sch != nullptr)
				sch->add_generator(this);
		}
	}

	template<typename _Ty>
	auto state_t<_Ty&>::future_await_resume() -> reference_type
	{
		std::scoped_lock __guard(this->_mtx);

		switch (this->_has_value.load(std::memory_order_acquire))
		{
		case result_type::None:
			std::rethrow_exception(std::make_exception_ptr(future_exception{ error_code::not_ready }));
			break;
		case result_type::Exception:
			std::rethrow_exception(std::move(this->_exception));
			break;
		default:
			break;
		}

		return static_cast<reference_type>(*this->_value);
	}

	template<typename _Ty>
	void state_t<_Ty&>::set_value_internal(reference_type val)
	{
		switch (_has_value.load(std::memory_order_acquire))
		{
		case result_type::Exception:
			_exception.~exception_ptr();
		default:
			this->_value = std::addressof(val);
			this->_has_value.store(result_type::Value, std::memory_order_release);
			break;
		}
	}

	template<typename _Ty>
	void state_t<_Ty&>::set_exception_internal(std::exception_ptr e)
	{
		switch (_has_value.load(std::memory_order_acquire))
		{
		case result_type::Exception:
			_exception = std::move(e);
			break;
		default:
			new (&this->_exception) std::exception_ptr(std::move(e));
			this->_has_value.store(result_type::Exception, std::memory_order_release);
			break;
		}
	}

	template<typename _Ty>
	void state_t<_Ty&>::set_value(reference_type val)
	{
		std::scoped_lock __guard(this->_mtx);
		set_value_internal(val);

		scheduler_t* sch = this->get_scheduler();
		if (sch != nullptr)
		{
			if (this->has_handler_skip_lock())
				sch->add_generator(this);
			else
				sch->del_final(this);
		}
	}

	template<typename _Ty>
	void state_t<_Ty&>::set_exception(std::exception_ptr e)
	{
		std::scoped_lock __guard(this->_mtx);
		set_exception_internal(std::move(e));

		scheduler_t* sch = this->get_scheduler();
		if (sch != nullptr)
		{
			if (this->has_handler_skip_lock())
				sch->add_generator(this);
			else
				sch->del_final(this);
		}
	}
}

