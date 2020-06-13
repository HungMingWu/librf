#pragma once

#if RESUMEF_ENABLE_CONCEPT
#include <concepts>
#endif

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

#if RESUMEF_ENABLE_CONCEPT

	template<typename T>
	concept _CallableT = std::invocable<T>;

	//template<typename T>
	//concept _GeneratorT = std::is_same_v<T, generator_t<T>>;

	template<typename T>
	concept _AwaitableT = requires(T&& v)
	{
		{ traits::get_awaitor(v) } ->_AwaitorT;
	};

	template<typename T>
	concept _WhenTaskT = _AwaitableT<T> || _CallableT<T>;

	template<typename T>
	concept _IteratorT = requires(T&& u, T&& v)
	{
		{ ++u } ->std::common_with<T>;
		{ u != v } ->std::same_as<bool>;
		{ *u };
	};

	template<typename T, typename E>
	concept _IteratorOfT = _IteratorT<T> && requires(T&& u)
	{
		{ *u } ->std::common_with<E&>;
	};

	template<typename T>
	concept _WhenIterT = _IteratorT<T> && requires(T&& u)
	{
		{ *u } ->_WhenTaskT;
	};

	template<typename T>
	concept _ContainerT = requires(T&& v)
	{
		{ std::begin(v) } ->_IteratorT;
		{ std::end(v) } ->_IteratorT;
		requires std::same_as<decltype(std::begin(v)), decltype(std::end(v))>;
	};

	template<typename T, typename E>
	concept _ContainerOfT = _ContainerT<T> && requires(T&& v)
	{
		{ *std::begin(v) } ->std::common_with<E&>;
		//requires std::is_same_v<E, remove_cvref_t<decltype(*std::begin(v))>>;
	};
	template <typename T>
	concept _PromiseT = traits::is_promise_v<T>;

#define COMMA_RESUMEF_ENABLE_IF_TYPENAME() 
#define COMMA_RESUMEF_ENABLE_IF(...) 
#define RESUMEF_ENABLE_IF(...) 
#define RESUMEF_REQUIRES(...) requires __VA_ARGS__

#else

#define _CallableT typename
//#define _GeneratorT typename
#define _AwaitableT typename
#define _WhenTaskT typename
#define _IteratorT typename
#define _IteratorOfT typename
#define _WhenIterT typename
#define _ContainerT typename
#define _ContainerOfT typename
#define _PromiseT typename

#define COMMA_RESUMEF_ENABLE_IF_TYPENAME() ,typename _EnableIf
#define COMMA_RESUMEF_ENABLE_IF(...) ,typename=std::enable_if_t<__VA_ARGS__>
#define RESUMEF_ENABLE_IF(...) typename=std::enable_if_t<__VA_ARGS__>
#define RESUMEF_REQUIRES(...) 

#endif

#if RESUMEF_ENABLE_CONCEPT
template<typename T>
concept _LockAssembleT = requires(T && v)
{
	{ v.size() };
	{ v[0] };
	{ v._Lock_ref(v[0]) };
	{ v._Try_lock_ref(v[0]) };
	{ v._Unlock_ref(v[0]) } ->std::same_as<void>;
	{ v._Yield() };
	{ v._ReturnValue() };
	{ v._ReturnValue(0) };
	requires std::is_integral_v<decltype(v.size())>;
};
#else
#define _LockAssembleT typename
#endif

}
