#pragma once

namespace resumef
{
	template<class node_type, class size_type = uint32_t>
	struct intrusive_link_queue
	{
		using node_ptr_type = std::add_pointer_t<node_type>;
	public:
		intrusive_link_queue();

		intrusive_link_queue(const intrusive_link_queue&) = delete;
		intrusive_link_queue(intrusive_link_queue&&) = default;
		intrusive_link_queue& operator =(const intrusive_link_queue&) = delete;
		intrusive_link_queue& operator =(intrusive_link_queue&&) = default;

		auto size() const noexcept->size_type;
		bool empty() const noexcept;
		void push_back(node_ptr_type node) noexcept;
		auto try_pop() noexcept->node_ptr_type;
	private:
		node_ptr_type _head;
		node_ptr_type _tail;

	#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
		std::atomic<size_type> m_count;
	#endif
	};

	template<class node_type, class size_type>
	intrusive_link_queue<node_type, size_type>::intrusive_link_queue()
		: _head(nullptr)
		, _tail(nullptr)
	#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
		, m_count(0)
	#endif
	{
	}

	template<class node_type, class size_type>
	auto intrusive_link_queue<node_type, size_type>::size() const noexcept->size_type
	{
	#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
		return m_count.load(std::memory_order_acquire);
	#else
		size_type count = 0;
		for (node_type* node = _head; node != nullptr; node = node->next)
			++count;
		return count;
	#endif // _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
	}

	template<class node_type, class size_type>
	bool intrusive_link_queue<node_type, size_type>::empty() const noexcept
	{
		return _head == nullptr;
	}

	template<class node_type, class size_type>
	void intrusive_link_queue<node_type, size_type>::push_back(node_ptr_type node) noexcept
	{
		assert(node != nullptr);

		node->_next = nullptr;
		if (_head == nullptr)
		{
			_head = _tail = node;
		}
		else
		{
			_tail->_next = node;
			_tail = node;
		}

	#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
		m_count.fetch_add(1, std::memory_order_acq_rel);
	#endif
	}

	template<class node_type, class size_type>
	auto intrusive_link_queue<node_type, size_type>::try_pop() noexcept->node_ptr_type
	{
		if (_head == nullptr)
			return nullptr;

		node_ptr_type node = _head;
		_head = node->_next;
		node->_next = nullptr;

		if (_tail == node)
		{
			assert(node->_next == nullptr);
			_tail = nullptr;
		}

	#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
		m_count.fetch_sub(1, std::memory_order_acq_rel);
	#endif

		return node;
	}
}