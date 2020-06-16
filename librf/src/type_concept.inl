#pragma once

namespace resumef
{
	template<typename T>
	concept _HasStateT = requires(T && v)
	{
		{ v._state };// ->traits::is_instance<counted_ptr>;
	};

	template<typename T>
	concept _FutureT = traits::_AwaitorT<T> && _HasStateT<T> && requires
	{
		typename T::value_type;
		typename T::state_type;
		typename T::promise_type;
	};

	template<typename T>
	concept _CallableT = traits::invocable<T>;

	template<typename T>
	concept _IteratorT = requires(T && u, T && v)
	{
		{ ++u }; //->traits::common_with<T>;
		{ u != v } -> traits::same_as<bool>;
		{ *u };
	};

	template<typename T, typename E>
	concept _IteratorOfT = _IteratorT<T> && requires(T && u)
	{
		{ *u } -> traits::same_as<E&>;
	};

	template<typename T>
	concept _AwaitableT = requires(T && v)
	{
		{ traits::get_awaitor(v) } -> traits::_AwaitorT;
	};

	template<typename T>
	concept _WhenTaskT = _AwaitableT<T> || _CallableT<T>;

	template<typename T>
	concept _WhenIterT = _IteratorT<T> && requires(T && u)
	{
		{ *u } -> _WhenTaskT;
	};

	template<typename T>
	concept _ContainerT = requires(T && v)
	{
		{ std::begin(v) } -> _IteratorT;
		{ std::end(v) } -> _IteratorT;
		requires traits::same_as<decltype(std::begin(v)), decltype(std::end(v))>;
	};

	template<typename T, typename E>
	concept _ContainerOfT = _ContainerT<T> && requires(T && v)
	{
		{ *std::begin(v) };
		requires std::is_same_v<E, remove_cvref_t<decltype(*std::begin(v))>>;
	};

	template<typename T>
	concept _GeneratorT = traits::is_instance_v<generator_t, T>;

	template <typename T>
	concept _PromiseT = traits::is_instance_v<promise_t, T>;

	template<typename T>
	concept _LockAssembleT = requires(T && v)
	{
		{ v.size() };
		{ v[0] };
		{ v._Lock_ref(v[0]) };
		{ v._Try_lock_ref(v[0]) };
		{ v._Unlock_ref(v[0]) } -> traits::same_as<void>;
		{ v._Yield() };
		{ v._ReturnValue() };
		{ v._ReturnValue(0) };
		requires std::is_integral_v<decltype(v.size())>;
	};

}
