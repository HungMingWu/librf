﻿#include "../librf.h"

namespace resumef
{
	namespace detail
	{
		mutex_impl::mutex_impl()
		{
		}

		void mutex_impl::unlock()
		{
			std::scoped_lock lock_(this->_lock);

			if (_owner != nullptr)
			{
				for (auto iter = _awakes.begin(); iter != _awakes.end(); )
				{
					auto awaker = *iter;
					iter = _awakes.erase(iter);

					if (awaker->awake(this, 1))
					{
						_owner = awaker;
						break;
					}

				}
				if (_awakes.size() == 0)
				{
					_owner = nullptr;
				}
			}
		}

		bool mutex_impl::lock_(const mutex_awaker_ptr& awaker)
		{
			assert(awaker);

			std::scoped_lock lock_(this->_lock);

			if (_owner == nullptr)
			{
				_owner = awaker;
				awaker->awake(this, 1);
				return true;
			}
			else
			{
				_awakes.push_back(awaker);
				return false;
			}
		}

		bool mutex_impl::try_lock_(const mutex_awaker_ptr& awaker)
		{
			assert(awaker);

			std::scoped_lock lock_(this->_lock);

			if (_owner == nullptr)
			{
				_owner = awaker;
				return true;
			}
			else
			{
				return false;
			}
		}

	}

namespace mutex_v1
{

	mutex_t::mutex_t()
		: _locker(std::make_shared<detail::mutex_impl>())
	{
	}

	future_t<bool> mutex_t::lock() const
	{
		awaitable_t<bool> awaitable;

		auto awaker = std::make_shared<detail::mutex_awaker>(
			[st = awaitable._state](detail::mutex_impl* e) -> bool
			{
				st->set_value(e ? true : false);
				return true;
			});
		_locker->lock_(awaker);

		return awaitable.get_future();
	}

	bool mutex_t::try_lock() const
	{
		auto dummy_awaker = std::make_shared<detail::mutex_awaker>(
			[](detail::mutex_impl*) -> bool
			{
				return true;
			});
		return _locker->try_lock_(dummy_awaker);
	}

	future_t<bool> mutex_t::try_lock_until_(const clock_type::time_point& tp) const
	{
		awaitable_t<bool> awaitable;

		auto awaker = std::make_shared<detail::mutex_awaker>(
			[st = awaitable._state](detail::mutex_impl* e) -> bool
			{
				st->set_value(e ? true : false);
				return true;
			});
		_locker->lock_(awaker);

		(void)this_scheduler()->timer()->add(tp,
			[awaker](bool)
			{
				awaker->awake(nullptr, 1);
			});

		return awaitable.get_future();
	}
}
}
