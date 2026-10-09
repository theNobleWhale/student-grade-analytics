#pragma once

#include <cstddef>
#include <deque>
#include <stdexcept>

/// Rolling (moving) average over the last N values.
///
/// Uses std::deque as a sliding window: push_back for the newest value,
/// pop_front to evict the oldest. Both are O(1), so every update is O(1)
/// regardless of window size (this is the collections.deque(maxlen=N) idea).
class RollingAverage {
private:
	std::size_t m_windowSize;
	std::deque<double> m_window;
	double m_sum = 0.0;

public:
	explicit RollingAverage(std::size_t windowSize)
		: m_windowSize(windowSize)
	{
		if (windowSize == 0)
			throw std::invalid_argument(
				"Rolling window size must be greater than 0");
	}

	/// Adds a value, evicts the oldest if the window is full,
	/// and returns the new average.
	double add(double value)
	{
		m_window.push_back(value);
		m_sum += value;

		if (m_window.size() > m_windowSize)
		{
			m_sum -= m_window.front();
			m_window.pop_front();
		}

		return average();
	}

	double average() const
	{
		if (m_window.empty()) return 0.0;
		return m_sum / static_cast<double>(m_window.size());
	}

	std::size_t size() const { return m_window.size(); }
	std::size_t windowSize() const { return m_windowSize; }
	bool full() const { return m_window.size() == m_windowSize; }
	const std::deque<double>& values() const { return m_window; }

	void clear()
	{
		m_window.clear();
		m_sum = 0.0;
	}
};
