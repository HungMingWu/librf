#pragma once

namespace resumef
{
	namespace traits
	{
		//is_coroutine_handle<T>
		//is_coroutine_handle_v<T>
		//�ж��ǲ���coroutine_handle<>����
		//
		//is_valid_await_suspend_return_v<T>
		//�ж��ǲ���awaitor��await_suspend()��������Ч����ֵ
		//
		//is_awaitor<T>
		//is_awaitor_v<T>
		//�ж��ǲ���һ��awaitor�淶��
		//һ��awaitor���Ա�co_await������Ҫ������coroutine��awaitor�����������ӿڹ淶
		//
		//is_future<T>
		//is_future_v<T>
		//�ж��ǲ���һ��librf��future�淶��
		//future����Ҫ����һ��awaitor�⣬��Ҫ������value_type/state_type/promise_type�������ͣ�
		//���Ҿ߱�counted_ptr<state_type>���͵�_state������
		//
		//is_promise<T>
		//is_promise_v<T>
		//�ж��ǲ���һ��librf��promise_t��
		//
		//is_generator<T>
		//is_generator_v<T>
		//�ж��ǲ���һ��librf��generator_t��
		//
		//is_state_pointer<T>
		//is_state_pointer_v<T>
		//�ж��ǲ���һ��librf��state_t���ָ�������ָ��
		//
		//has_state<T>
		//has_state_v<T>
		//�ж��Ƿ����_state�ĳ�Ա����
		//
		//get_awaitor<T>(T&&t)
		//ͨ��T����䱻co_await���awaitor
		//
		//awaitor_traits<T>
		//���һ��awaitor��������
		//	type:awaitor������
		//	value_type:awaitor::await_resume()�ķ���ֵ����
		//
		//is_awaitable<T>
		//is_awaitable_v<T>
		//�ж��Ƿ���Ա�co_await������������һ��awaitor��Ҳ�����������˳�Ա������T::operator co_await()�����߱�������ȫ�ֵ�operator co_awaitor(T)
		//
		//is_iterator<T>
		//is_iterator_v<T>
		//�ж��ǲ���һ��֧���������ĵ�����
		//
		//is_iterator_of_v<T, E>
		//�ж��ǲ���һ��֧���������ĵ����������ҵ�����ͨ�� operator *()���ص������� E&��
		//
		//is_container<T>
		//is_container_v<T>
		//�ж��ǲ���һ�����������������������顣
		//
		//is_container_of<T, E>
		//is_container_of_v<T, E>
		//�ж��ǲ���һ�����������������������顣��Ԫ��������E��

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

		// STRUCT TEMPLATE common_reference
		template <class...>
		struct common_reference;

		// ALIAS TEMPLATE common_reference_t
		template <class... _Types>
		using common_reference_t = typename common_reference<_Types...>::type;

		// N4810 [meta.trans.other]/5.1: "If sizeof...(T) is zero ..."
		template <>
		struct common_reference<> {};

		// N4810 [meta.trans.other]/5.2: "...if sizeof...(T) is one ..."
		template <class _Ty>
		struct common_reference<_Ty> {
			using type = _Ty;
		};

		template <class _Ty1, class _Ty2>
		concept common_reference_with =
			requires {
			typename common_reference_t<_Ty1, _Ty2>;
			typename common_reference_t<_Ty2, _Ty1>;
		}
		&& same_as<common_reference_t<_Ty1, _Ty2>, common_reference_t<_Ty2, _Ty1>>
			&& convertible_to<_Ty1, common_reference_t<_Ty1, _Ty2>>
			&& convertible_to<_Ty2, common_reference_t<_Ty1, _Ty2>>;

		template <class _Ty1, class _Ty2>
		concept common_with =
			requires {
			typename std::common_type_t<_Ty1, _Ty2>;
			typename std::common_type_t<_Ty2, _Ty1>;
			requires same_as<std::common_type_t<_Ty1, _Ty2>, std::common_type_t<_Ty2, _Ty1>>;
			static_cast<std::common_type_t<_Ty1, _Ty2>>(std::declval<_Ty1>());
			static_cast<std::common_type_t<_Ty1, _Ty2>>(std::declval<_Ty2>());
		}
		&& common_reference_with<std::add_lvalue_reference_t<const _Ty1>, std::add_lvalue_reference_t<const _Ty2>>
			&& common_reference_with<std::add_lvalue_reference_t<std::common_type_t<_Ty1, _Ty2>>,
			common_reference_t<std::add_lvalue_reference_t<const _Ty1>, std::add_lvalue_reference_t<const _Ty2>>>;
		///
		template <typename ...Ts>
		constexpr bool is_instance_v()
		{
			return is_instance<Ts...>::value;
		}

		template<class _Ty>
		concept is_coroutine_handle_v = is_instance_v<_Ty, coroutine_handle>();

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

		template<class _Ty>
		concept is_promise_v = is_instance_v<_Ty, promise_t>();

		template<class _Ty>
		concept is_generator_v = is_instance<_Ty, generator_t>::value;

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

		template<class _Ty, class = std::void_t<>>
		struct is_iterator : std::false_type {};
		template<class _Ty>
		struct is_iterator
			<_Ty,
				std::void_t<
					decltype(std::declval<_Ty>() + 1)
					, decltype(std::declval<_Ty>() != std::declval<_Ty>())
					, decltype(*std::declval<_Ty>())
				>
			>
			: std::true_type{};
		template<class _Ty>
		constexpr bool is_iterator_v = is_iterator<_Ty>::value;
		template<class _Ty, class _Ety>
		constexpr bool is_iterator_of_v = std::conjunction<
				is_iterator<_Ty>
				, std::is_same<_Ety&, decltype(*std::declval<_Ty>())>
			>::value;

		template<class _Ty, class = std::void_t<>>
		struct is_container : std::false_type {};
		template<class _Ty>
		struct is_container
			<_Ty,
				std::void_t<
					decltype(std::begin(std::declval<_Ty>()))
					, decltype(std::end(std::declval<_Ty>()))
				>
			>
			: is_iterator<decltype(std::begin(std::declval<_Ty>()))> {};
		template<class _Ty, size_t _Size>
		struct is_container<_Ty[_Size]> : std::true_type {};
		template<class _Ty, size_t _Size>
		struct is_container<_Ty(&)[_Size]> : std::true_type {};
		template<class _Ty, size_t _Size>
		struct is_container<_Ty(&&)[_Size]> : std::true_type {};

		template<class _Ty>
		constexpr bool is_container_v = is_container<remove_cvref_t<_Ty>>::value;

		template<class _Ty, class _Ety, class = std::void_t<>>
		struct is_container_of : std::false_type {};
		template<class _Ty, class _Ety>
		struct is_container_of
			<_Ty, _Ety,
				std::void_t<
					decltype(std::begin(std::declval<_Ty>()))
					, decltype(std::end(std::declval<_Ty>()))
				>
			>
			: std::conjunction<
				is_iterator<decltype(std::begin(std::declval<_Ty>()))>
				, std::is_same<_Ety, remove_cvref_t<decltype(*std::begin(std::declval<_Ty>()))>>
			> {};
		template<class _Ty, size_t _Size>
		struct is_container_of<_Ty[_Size], _Ty> : std::true_type {};
		template<class _Ty, size_t _Size>
		struct is_container_of<_Ty(&)[_Size], _Ty> : std::true_type {};
		template<class _Ty, size_t _Size>
		struct is_container_of<_Ty(&&)[_Size], _Ty> : std::true_type {};

		//template<class _Ty, class _Ety>
		//constexpr bool is_container_of_v = is_container_of<remove_cvref_t<_Ty>, _Ety>::value;
	}
}
