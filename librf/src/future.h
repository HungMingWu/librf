﻿
#pragma once

namespace resumef
{

	/**
	 * @brief 用于resumef协程的返回值。
	 * @details 由于coroutines的限制，协程的返回值必须明确申明，而不能通过auto推导。\n
	 * 用在恢复函数(resumeable function)里，支持co_await和co_yield。\n
	 * 用在可等待函数(awaitable function)里，与awaitable_t<>配套使用。
	 */
	template<class _Ty>
	struct [[nodiscard]] future_t
	{
		using value_type = _Ty;
		using state_type = state_t<value_type>;
		using promise_type = promise_t<value_type>;
		using future_type = future_t<value_type>;

		counted_ptr<state_type> _state;

		future_t(counted_ptr<state_type> _st) noexcept
			:_state(std::move(_st)) {}
		future_t(const future_t&) noexcept = default;
		future_t(future_t&&) noexcept = default;

		future_t& operator = (const future_t&) noexcept = default;
		future_t& operator = (future_t&&) = default;

		bool await_ready() const noexcept
		{
			return _state->future_await_ready();
		}

		template<_PromiseT Promise>
		void await_suspend(coroutine_handle<Promise> handler)
		{
			_state->future_await_suspend(handler);
		}

		value_type await_resume() const
		{
			return _state->future_await_resume();
		}
	};
}
