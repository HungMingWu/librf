//Э�̵Ķ�ʱ��
//�����ʱ����ȡ���ˣ������ timer_canceled_exception �쳣
//��ʹ��co_await������sleep_for/sleep_until���Ǵ�����÷��������ܴﵽ�ȴ���Ŀ�ġ��������������һ����Ч�Ķ�ʱ��������ޱ�Ҫ���ڴ�ʹ��
//
#pragma once

namespace resumef
{
	/**
	 * @brief Э��ר�õ�˯�߹��ܡ�
	 * @details ����ʹ�ò���ϵͳ�ṩ��sleep���ܣ���Ϊ������Э�̡�\n
	 * �˺������������̣߳���������ǰЭ�̹���ֱ��ָ��ʱ�̡�\n
	 * �侫�ȣ�ȡ���������ѭ���ľ��ȣ��Լ�std::chrono::system_clock�ľ��ȡ������֮��������ΪֻҪѭ�����죬���ȵ�100ns��
	 * @return [co_await] void
	 * @throw timer_canceled_exception �����ʱ����ȡ�������״��쳣��
	 */
	future_t<> sleep_until_(std::chrono::system_clock::time_point tp_, scheduler_t& scheduler_);

	/**
	 * @brief Э��ר�õ�˯�߹��ܡ�
	 * @see �ο�sleep_until_()����\n
	 * @return [co_await] void
	 * @throw timer_canceled_exception �����ʱ����ȡ�������״��쳣��
	 */
	inline future_t<> sleep_for_(std::chrono::system_clock::duration dt_, scheduler_t& scheduler_)
	{
		return sleep_until_(std::chrono::system_clock::now() + dt_, scheduler_);
	}

	/**
	 * @brief Э��ר�õ�˯�߹��ܡ�
	 * @see �ο�sleep_until_()����\n
	 * @return [co_await] void
	 * @throw timer_canceled_exception �����ʱ����ȡ�������״��쳣��
	 */
	template <_ChronoDurationT Duration>
	inline future_t<> sleep_for(Duration dt_, scheduler_t& scheduler_)
	{
		return sleep_for_(std::chrono::duration_cast<std::chrono::system_clock::duration>(dt_), scheduler_);
	}

	/**
	 * @brief Э��ר�õ�˯�߹��ܡ�
	 * @see �ο�sleep_until_()����\n
	 * @return [co_await] void
	 * @throw timer_canceled_exception �����ʱ����ȡ�������״��쳣��
	 */
	template <_ChronoTimePointT TimePoint>
	inline future_t<> sleep_until(TimePoint tp_, scheduler_t& scheduler_)
	{
		return sleep_until_(std::chrono::time_point_cast<std::chrono::system_clock::duration>(tp_), scheduler_);
	}

	/**
	 * @brief Э��ר�õ�˯�߹��ܡ�
	 * @see �ο�sleep_until_()����\n
	 * @return [co_await] void
	 * @throw timer_canceled_exception �����ʱ����ȡ�������״��쳣��
	 */
	template <_ChronoDurationT Duration>
	inline future_t<> sleep_for(Duration dt_)
	{
		scheduler_t* sch = current_scheduler();
		co_await sleep_for_(std::chrono::duration_cast<std::chrono::system_clock::duration>(dt_), *sch);
	}

	/**
	 * @brief Э��ר�õ�˯�߹��ܡ�
	 * @see �ο�sleep_until_()����\n
	 * @return [co_await] void
	 * @throw timer_canceled_exception �����ʱ����ȡ�������״��쳣��
	 */
	template<_ChronoTimePointT TimePoint>
	inline future_t<> sleep_until(TimePoint tp_)
	{
		scheduler_t* sch = current_scheduler();
		co_await sleep_until_(std::chrono::time_point_cast<std::chrono::system_clock::duration>(tp_), *sch);
	}

	/**
	 * @brief Э��ר�õ�˯�߹��ܡ�
	 * @see ��ͬ����sleep_for(dt)\n
	 * @return [co_await] void
	 * @throw timer_canceled_exception �����ʱ����ȡ�������״��쳣��
	 */
	template <class Rep, class Period>
	inline future_t<> operator co_await(std::chrono::duration<Rep, Period> dt_)
	{
		scheduler_t* sch = current_scheduler();
		co_await sleep_for(dt_, *sch);
	}

}