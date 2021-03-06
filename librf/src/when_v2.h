﻿#pragma once

namespace resumef
{
	using any_t = std::any;
	using std::any_cast;
}

//纠结过when_any的返回值，是选用index + std::any，还是选用std::variant<>。最终选择了std::any。
//std::variant<>存在第一个元素不能默认构造的问题，需要使用std::monostate来占位，导致下标不是从0开始。
//而且，std::variant<>里面存在类型重复的问题，好几个操作都是病态的
//最最重要的，要统一ranged when_any的返回值，还得做一个运行时通过下标设置std::variant<>的东西
//std::any除了内存布局不太理想，其他方面几乎没缺点（在此应用下）

namespace resumef
{
#ifndef DOXYGEN_SKIP_PROPERTY
	using when_any_pair = std::pair<intptr_t, any_t>;
	using when_any_pair_ptr = std::shared_ptr<when_any_pair>;

	namespace detail
	{
		struct state_when_t : public state_base_t
		{
			state_when_t(intptr_t counter_);

			void on_cancel() noexcept;
			bool on_notify_one();
			bool on_timeout();

			//将自己加入到通知链表里
			template<_PromiseT Promise>
			scheduler_t* on_await_suspend(coroutine_handle<Promise> handler) noexcept
			{
				Promise& promise = handler.promise();
				auto* parent_state = promise.get_state();
				scheduler_t* sch = parent_state->get_scheduler();

				this->_scheduler = sch;
				this->_coro = handler;

				return sch;
			}

			spinlock _lock;
			std::atomic<intptr_t> _counter;
		};

		template<class value_type>
		struct [[nodiscard]] when_future_t
		{
			using state_type = detail::state_when_t;
			using promise_type = promise_t<value_type>;
			using future_type = when_future_t<value_type>;

			counted_ptr<state_type> _state;
			std::shared_ptr<value_type> _values;

			when_future_t(intptr_t count_) noexcept
				: _state(new state_type(count_))
				, _values(std::make_shared<value_type>())
			{
			}

			bool await_ready() noexcept
			{
				return _state->_counter.load(std::memory_order_relaxed) == 0;
			}

			template<_PromiseT Promise>
			void await_suspend(coroutine_handle<Promise> handler)
			{
				_state->on_await_suspend(handler);
			}

			value_type await_resume() noexcept(std::is_nothrow_move_constructible_v<value_type>)
			{
				return std::move(*_values);
			}
		};


		using ignore_type = std::remove_const_t<decltype(std::ignore)>;

		template<class _Ty>
		struct convert_void_2_ignore
		{
			using type = std::remove_reference_t<_Ty>;
			using value_type = type;
		};
		template<>
		struct convert_void_2_ignore<void>
		{
			using type = void;
			using value_type = ignore_type;
		};

		template<class _Ty, bool = _CallableT<_Ty>>
		struct awaitor_result_impl2
		{
			using value_type = typename convert_void_2_ignore<
				typename traits::awaitor_traits<_Ty>::value_type
			>::value_type;
		};
		template<class _Ty>
		struct awaitor_result_impl2<_Ty, true> : awaitor_result_impl2<decltype(std::declval<_Ty>()()), false> {};

		template<_WhenTaskT _Ty>
		using awaitor_result_t = typename awaitor_result_impl2<std::remove_reference_t<_Ty>>::value_type;

		template<_WhenTaskT _Awaitable>
		decltype(auto) when_real_awaitor(_Awaitable&& awaitor)
		{
			if constexpr (_CallableT<_Awaitable>)
				return awaitor();
			else
				return std::forward<_Awaitable>(awaitor);
		}

		template<_WhenTaskT _Awaitable, class _Ty>
		future_t<> when_all_connector(state_when_t* state, _Awaitable task, _Ty& value)
		{
			decltype(auto) awaitor = when_real_awaitor(task);

			if constexpr (std::is_same_v<_Ty, ignore_type>)
				co_await awaitor;
			else
				value = co_await awaitor;
			state->on_notify_one();
		};

		template<class _Tup, class _AwaitTuple, std::size_t... I>
		inline void when_all_one__(scheduler_t& sch, state_when_t* state, _Tup& values,
			_AwaitTuple&& awaitables,
			std::index_sequence<I...>)
		{
			(void)std::initializer_list<int> {
				(sch + when_all_connector(state, std::get<I>(awaitables), std::get<I>(values)), 0)...
			};
		}

		template<class _Val, _WhenIterT _Iter>
		inline void when_all_range__(scheduler_t& sch, state_when_t* state, std::vector<_Val> & values, _Iter begin, _Iter end)
		{
			using _Awaitable = std::remove_reference_t<decltype(*begin)>;

			intptr_t _Idx = 0;
			for (; begin != end; ++begin, ++_Idx)
			{
				sch + when_all_connector<_Awaitable, _Val>(state, *begin, values[_Idx]);
			}
		}

//-----------------------------------------------------------------------------------------------------------------------------------------

		template<_WhenTaskT _Awaitable>
		future_t<> when_any_connector(counted_ptr<state_when_t> state, _Awaitable task, when_any_pair_ptr value, intptr_t idx)
		{
			assert(idx >= 0);

			auto awaitor = when_real_awaitor(task);

			using value_type = awaitor_result_t<decltype(awaitor)>;

			if constexpr (std::is_same_v<value_type, ignore_type>)
			{
				co_await awaitor;

				intptr_t oldValue = -1;
				if (reinterpret_cast<std::atomic<intptr_t>&>(value->first).compare_exchange_strong(oldValue, idx))
				{
					state->on_notify_one();
				}
			}
			else
			{
				decltype(auto) result = co_await awaitor;

				intptr_t oldValue = -1;
				if (reinterpret_cast<std::atomic<intptr_t>&>(value->first).compare_exchange_strong(oldValue, idx))
				{
					value->second = std::move(result);

					state->on_notify_one();
				}
			}
		};

		template <class _Awaitable, std::size_t... I>
		inline void when_any_one__(scheduler_t& sch, state_when_t* state, when_any_pair_ptr value,
			_Awaitable&& awaitable,
			std::index_sequence<I...>)
		{
			(void)std::initializer_list<int> {
				(sch + when_any_connector(state, std::get<I>(awaitable), value, I), 0)...
			};
		}

		template<_WhenIterT _Iter>
		inline void when_any_range__(scheduler_t& sch, state_when_t* state, when_any_pair_ptr value, _Iter begin, _Iter end)
		{
			using _Awaitable = std::remove_reference_t<decltype(*begin)>;

			intptr_t _Idx = 0;
			for (; begin != end; ++begin, ++_Idx)
			{
				sch + when_any_connector<_Awaitable>(state, *begin, value, static_cast<intptr_t>(_Idx));
			}
		}
	}

#endif	//DOXYGEN_SKIP_PROPERTY

#ifndef DOXYGEN_SKIP_PROPERTY
inline namespace when_v2
{
#else
	/**
	 * @brief 目前不知道怎么在doxygen里面能搜集到全局函数的文档。故用一个结构体来欺骗doxygen。
	 * @details 其下的所有成员函数，均是全局函数。
	 */
	struct when_{
#endif

	/**
	 * @brief 等待所有的可等待对象完成，不定参数版。
	 * @param sch 当前协程的调度器。
	 * @param args... 所有的可等待对象。要么是_AwaitableT<>类型，要么是返回_AwaitableT<>类型的函数(对象)。
	 * @retval [co_await] std::tuple<...>。每个可等待对象的返回值，逐个存入到std::tuple<...>里面。void 返回值，存的是std::ignore。
	 */
	template <_WhenTaskT... _Awaitable>
	auto when_all(scheduler_t& sch, _Awaitable&&... args)
		-> detail::when_future_t<std::tuple<detail::awaitor_result_t<_Awaitable>...> >
	{
		using tuple_type = std::tuple<detail::awaitor_result_t<_Awaitable>...>;
		auto await_tuple = std::make_tuple(std::forward<_Awaitable>(args)...);
		detail::when_future_t<tuple_type> awaitor{ sizeof...(_Awaitable) };
		detail::when_all_one__<tuple_type>(sch, awaitor._state.get(), *awaitor._values,
			await_tuple, std::make_index_sequence<std::tuple_size_v<tuple_type>>());

		return awaitor;
	}

	/**
	 * @brief 等待所有的可等待对象完成，迭代器版。
	 * @param sch 当前协程的调度器。
	 * @param begin 可等待对象容器的起始迭代器。迭代器指向的，要么是_AwaitableT<>类型，要么是返回_AwaitableT<>类型的函数(对象)。
	 * @param end 可等待对象容器的结束迭代器。
	 * @retval [co_await] std::vector<>。每个可等待对象的返回值，逐个存入到std::vector<>里面。void 返回值，存的是std::ignore。
	 */
	template<_WhenIterT _Iter>
	auto when_all(scheduler_t& sch, _Iter begin, _Iter end)
		-> detail::when_future_t<std::vector<detail::awaitor_result_t<decltype(*std::declval<_Iter>())> > >
	{
		using value_type = detail::awaitor_result_t<decltype(*std::declval<_Iter>())>;
		using vector_type = std::vector<value_type>;

		detail::when_future_t<vector_type> awaitor{ std::distance(begin, end) };
		awaitor._values->resize(end - begin);
		when_all_range__(sch, awaitor._state.get(), *awaitor._values, begin, end);

		return awaitor;
	}

	/**
	 * @brief 等待所有的可等待对象完成，容器版。
	 * @param sch 当前协程的调度器。
	 * @param cont 存访可等待对象的容器。容器内存放的，要么是_AwaitableT<>类型，要么是返回_AwaitableT<>类型的函数(对象)。
	 * @retval [co_await] std::vector<>。每个可等待对象的返回值，逐个存入到std::vector<>里面。void 返回值，存的是std::ignore。
	 */
	template<_ContainerT _Cont>
	decltype(auto) when_all(scheduler_t& sch, _Cont& cont)
	{
		return when_all(sch, std::begin(cont), std::end(cont));
	}

	/**
	 * @brief 等待所有的可等待对象完成，不定参数版。
	 * @details 当前协程的调度器通过current_scheduler()宏获得，与带调度器参数的版本相比，多一次resumeable function构造，效率可能低一点。
	 * @param args... 所有的可等待对象。要么是_AwaitableT<>类型，要么是返回_AwaitableT<>类型的函数(对象)。
	 * @retval [co_await] std::tuple<...>。每个可等待对象的返回值，逐个存入到std::tuple<...>里面。void 返回值，存的是std::ignore。
	 */
	template<_WhenTaskT... _Awaitable>
	auto when_all(_Awaitable&&... args)
		-> future_t<std::tuple<detail::awaitor_result_t<_Awaitable>...>>
	{
		scheduler_t* sch = current_scheduler();
		co_return co_await when_all(*sch, std::forward<_Awaitable>(args)...);
	}

	/**
	 * @brief 等待所有的可等待对象完成，迭代器版。
	 * @details 当前协程的调度器通过current_scheduler()宏获得，与带调度器参数的版本相比，多一次resumeable function构造，效率可能低一点。
	 * @param begin 可等待对象容器的起始迭代器。迭代器指向的，要么是_AwaitableT<>类型，要么是返回_AwaitableT<>类型的函数(对象)。
	 * @param end 可等待对象容器的结束迭代器。
	 * @retval [co_await] std::vector<>。每个可等待对象的返回值，逐个存入到std::vector<>里面。void 返回值，存的是std::ignore。
	 */
	template<_WhenIterT _Iter>
	auto when_all(_Iter begin, _Iter end)
		-> future_t<std::vector<detail::awaitor_result_t<decltype(*begin)>>>
	{
		scheduler_t* sch = current_scheduler();
		co_return co_await when_all(*sch, begin, end);
	}

	/**
	 * @brief 等待所有的可等待对象完成，容器版。
	 * @details 当前协程的调度器通过current_scheduler()宏获得，与带调度器参数的版本相比，多一次resumeable function构造，效率可能低一点。
	 * @param cont 存访可等待对象的容器。容器内存放的，要么是_AwaitableT<>类型，要么是返回_AwaitableT<>类型的函数(对象)。
	 * @retval [co_await] std::vector<>。每个可等待对象的返回值，逐个存入到std::vector<>里面。void 返回值，存的是std::ignore。
	 */
	template<_ContainerT _Cont>
	auto when_all(_Cont&& cont)
		-> future_t<std::vector<detail::awaitor_result_t<decltype(*std::begin(cont))>>>
	{
		return when_all(std::begin(cont), std::end(cont));
	}



	/**
	 * @brief 等待任一的可等待对象完成，不定参数版。
	 * @param sch 当前协程的调度器。
	 * @param args... 所有的可等待对象。要么是_AwaitableT<>类型，要么是返回_AwaitableT<>类型的函数(对象)。
	 * @retval [co_await] std::pair<intptr_t, std::any>。第一个值指示哪个对象完成了，第二个值存访的对应的返回数据。
	 */
	template<_WhenTaskT... _Awaitable>
	auto when_any(scheduler_t& sch, _Awaitable&&... args)
		-> detail::when_future_t<when_any_pair>
	{
#pragma warning(disable : 6326)	//warning C6326: Potential comparison of a constant with another constant.
		detail::when_future_t<when_any_pair> awaitor{ sizeof...(_Awaitable) > 0 ? 1 : 0 };
#pragma warning(default : 6326)
		auto await_tuple = std::make_tuple(std::forward<_Awaitable>(args)...);
		awaitor._values->first = -1;
		detail::when_any_one__(sch, awaitor._state.get(), awaitor._values, await_tuple,
			std::make_index_sequence<sizeof...(_Awaitable)>());

		return awaitor;
	}

	/**
	 * @brief 等待任一的可等待对象完成，迭代器版。
	 * @param sch 当前协程的调度器。
	 * @param begin 可等待对象容器的起始迭代器。迭代器指向的，要么是_AwaitableT<>类型，要么是返回_AwaitableT<>类型的函数(对象)。
	 * @param end 可等待对象容器的结束迭代器。
	 * @retval [co_await] std::pair<intptr_t, std::any>。第一个值指示哪个对象完成了，第二个值存访的对应的返回数据。
	 */
	template<_WhenIterT _Iter>
	auto when_any(scheduler_t& sch, _Iter begin, _Iter end)
		-> detail::when_future_t<when_any_pair>
	{
		detail::when_future_t<when_any_pair> awaitor{ begin == end ? 0 : 1 };
		awaitor._values->first = -1;
		detail::when_any_range__<_Iter>(sch, awaitor._state.get(), awaitor._values, begin, end);

		return awaitor;
	}

	/**
	 * @brief 等待任一的可等待对象完成，容器版。
	 * @param sch 当前协程的调度器。
	 * @param cont 存访可等待对象的容器。容器内存放的，要么是_AwaitableT<>类型，要么是返回_AwaitableT<>类型的函数(对象)。
	 * @retval [co_await] std::pair<intptr_t, std::any>。第一个值指示哪个对象完成了，第二个值存访的对应的返回数据。
	 */
	template<_ContainerT _Cont>
	auto when_any(scheduler_t& sch, _Cont& cont)
		-> detail::when_future_t<when_any_pair>
	{
		return when_any(sch, std::begin(cont), std::end(cont));
	}

	/**
	 * @brief 等待任一的可等待对象完成，不定参数版。
	 * @details 当前协程的调度器通过current_scheduler()宏获得，与带调度器参数的版本相比，多一次resumeable function构造，效率可能低一点。
	 * @param args... 所有的可等待对象。要么是_AwaitableT<>类型，要么是返回_AwaitableT<>类型的函数(对象)。
	 * @retval [co_await] std::pair<intptr_t, std::any>。第一个值指示哪个对象完成了，第二个值存访的对应的返回数据。
	 */
	template<_WhenTaskT... _Awaitable>
	auto when_any(_Awaitable&&... args)
		-> future_t<when_any_pair>
	{
		scheduler_t* sch = current_scheduler();
		co_return co_await when_any(*sch, std::forward<_Awaitable>(args)...);
	}

	/**
	 * @brief 等待任一的可等待对象完成，迭代器版。
	 * @details 当前协程的调度器通过current_scheduler()宏获得，与带调度器参数的版本相比，多一次resumeable function构造，效率可能低一点。
	 * @param begin 可等待对象容器的起始迭代器。迭代器指向的，要么是_AwaitableT<>类型，要么是返回_AwaitableT<>类型的函数(对象)。
	 * @param end 可等待对象容器的结束迭代器。
	 * @retval [co_await] std::pair<intptr_t, std::any>。第一个值指示哪个对象完成了，第二个值存访的对应的返回数据。
	 */
	template<_WhenIterT _Iter>
	auto when_any(_Iter begin, _Iter end) 
		-> future_t<when_any_pair>
	{
		scheduler_t* sch = current_scheduler();
		co_return co_await when_any(*sch, begin, end);
	}

	/**
	 * @brief 等待任一的可等待对象完成，容器版。
	 * @details 当前协程的调度器通过current_scheduler()宏获得，与带调度器参数的版本相比，多一次resumeable function构造，效率可能低一点。
	 * @param cont 存访可等待对象的容器。容器内存放的，要么是_AwaitableT<>类型，要么是返回_AwaitableT<>类型的函数(对象)。
	 * @retval [co_await] std::pair<intptr_t, std::any>。第一个值指示哪个对象完成了，第二个值存访的对应的返回数据。
	 */
	template <_ContainerT _Cont>
	auto when_any(_Cont&& cont)
		-> future_t<when_any_pair>
	{
		return when_any(std::begin(cont), std::end(cont));
	}

}
}
