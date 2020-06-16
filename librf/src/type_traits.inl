#pragma once

namespace resumef
{
	namespace traits
	{
		template <template <class, class...> class, class>
		struct is_instance : public std::false_type {};

		template <template <class, class...> class U, class...Ts>
		struct is_instance<U, U<Ts...>> : public std::true_type {};

		template <template <class, class...> class U, class ...Ts>
		concept is_instance_v = is_instance<U, Ts...>::value;

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

		template<class _Ty>
		concept is_coroutine_handle_v = is_instance_v<coroutine_handle, _Ty>;

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
