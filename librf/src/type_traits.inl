#pragma once

namespace resumef
{
	namespace traits
	{
		//is_coroutine_handle<T>
		//is_coroutine_handle_v<T>
		//判断是不是coroutine_handle<>类型
		//
		//is_valid_await_suspend_return_v<T>
		//判断是不是awaitor的await_suspend()函数的有效返回值
		//
		//is_awaitor<T>
		//is_awaitor_v<T>
		//判断是不是一个awaitor规范。
		//一个awaitor可以被co_await操作，要求满足coroutine的awaitor的三个函数接口规范
		//
		//is_future<T>
		//is_future_v<T>
		//判断是不是一个librf的future规范。
		//future除了要求是一个awaitor外，还要求定义了value_type/state_type/promise_type三个类型，
		//并且具备counted_ptr<state_type>类型的_state变量。
		//
		//is_state_pointer<T>
		//is_state_pointer_v<T>
		//判断是不是一个librf的state_t类的指针或智能指针
		//
		//has_state<T>
		//has_state_v<T>
		//判断是否具有_state的成员变量
		//
		//get_awaitor<T>(T&&t)
		//通过T获得其被co_await后的awaitor
		//
		//awaitor_traits<T>
		//获得一个awaitor的特征。
		//	type:awaitor的类型
		//	value_type:awaitor::await_resume()的返回值类型
		//
		//is_awaitable<T>
		//is_awaitable_v<T>
		//判断是否可以被co_await操作。可以是一个awaitor，也可以是重载了成员变量的T::operator co_await()，或者被重载了全局的operator co_awaitor(T)
		//
		//is_iterator<T>
		//is_iterator_v<T>
		//判断是不是一个支持向后迭代的迭代器
		//
		//is_iterator_of_v<T, E>
		//判断是不是一个支持向后迭代的迭代器，并且迭代器通过 operator *()返回的类型是 E&。
		//
		//is_container<T>
		//is_container_v<T>
		//判断是不是一个封闭区间的容器，或者数组。
		//
		//is_container_of<T, E>
		//is_container_of_v<T, E>
		//判断是不是一个封闭区间的容器，或者数组。其元素类型是E。

		template <class, template <class, class...> class>
		struct is_instance : public std::false_type {};

		template <class...Ts, template <class, class...> class U>
		struct is_instance<U<Ts...>, U> : public std::true_type {};

		/// Copy from VC
		template <class _Ty1, class _Ty2>
		concept _Same_impl = std::is_same_v<_Ty1, _Ty2>;

		template <class _Ty1, class _Ty2>
		concept same_as = _Same_impl<_Ty1, _Ty2> && _Same_impl<_Ty2, _Ty1>;

		// CONCEPT convertible_to
		template <class _From, class _To>
		concept convertible_to = __is_convertible_to(_From, _To)
			&& requires(std::add_rvalue_reference_t<_From>(&_Fn)()) {
			static_cast<_To>(_Fn());
		};

		///
		template <typename ...Ts>
		constexpr bool is_instance_v()
		{
			return is_instance<Ts...>::value;
		}

		template<class _Ty>
		concept is_coroutine_handle_v = is_instance<_Ty, coroutine_handle>::value;

		template<class _Ty>
		constexpr bool is_valid_await_suspend_return_v = std::is_void_v<_Ty> || std::is_same_v<_Ty, bool> || is_coroutine_handle_v<_Ty>;

		template<class _Ty>
		concept _AwaitorT = requires (_Ty v) {
			{ v.await_ready() } -> same_as<bool>;
			{ v.await_suspend(std::declval<std::experimental::coroutine_handle<promise_t<>>>()) };
			{ v.await_resume() };
			requires traits::is_valid_await_suspend_return_v<
				decltype(v.await_suspend(std::declval<std::experimental::coroutine_handle<promise_t<>>>()))
			>;
		};

		template <class _FTy, class... _ArgTys>
		concept invocable = requires(_FTy && _Fn, _ArgTys &&... _Args) {
			std::invoke(static_cast<_FTy&&>(_Fn), static_cast<_ArgTys&&>(_Args)...);
		};

		//copy from cppcoro
		namespace detail
		{
			template<class T>
			auto get_awaitor_impl(T&& value, int) noexcept(noexcept(static_cast<T&&>(value).operator co_await()))
				-> decltype(static_cast<T&&>(value).operator co_await())
			{
				return static_cast<T&&>(value).operator co_await();
			}
			template<class T>
			auto get_awaitor_impl(T&& value, long) noexcept(noexcept(operator co_await(static_cast<T&&>(value))))
				-> decltype(operator co_await(static_cast<T&&>(value)))
			{
				return operator co_await(static_cast<T&&>(value));
			}
			template<_AwaitorT T>
			T&& get_awaitor_impl(T&& value, std::any) noexcept
			{
				return static_cast<T&&>(value);
			}
		}

		template<class T>
		auto get_awaitor(T&& value) noexcept(noexcept(detail::get_awaitor_impl(static_cast<T&&>(value), 123)))
			-> decltype(detail::get_awaitor_impl(static_cast<T&&>(value), 123))
		{
			return detail::get_awaitor_impl(static_cast<T&&>(value), 123);
		}

		template <typename T>
		concept get_awaitable = requires(T t) {
			{ get_awaitor(t) };
		};

		template<class _Ty, class = std::void_t<>>
		struct awaitor_traits{};

		template<get_awaitable _Ty>
		struct awaitor_traits<_Ty>
		{
			using type = decltype(get_awaitor(std::declval<_Ty>()));
			using value_type = decltype(std::declval<type>().await_resume());
		};

		template<class _Ty, class = std::void_t<>>
		struct is_awaitable : std::false_type {};

		template<get_awaitable _Ty>
		struct is_awaitable<_Ty> : std::true_type {};

		template<typename _Ty>
		concept is_awaitable_v = is_awaitable<_Ty>::value;

	}
}
