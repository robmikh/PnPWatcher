#pragma once

template <typename T>
class RingList
{
public:
	RingList(size_t max = 10)
	{
		m_start = 0;
		m_max = max;
		m_vec = std::vector<T>();
		m_vec.reserve(m_max);
	}

	bool Add(T item)
	{
		auto index = (m_start + m_count) % m_max;
		bool adjusted = index == m_start && m_count != 0;
		m_count++;
		if (adjusted)
		{
			m_start = (m_start + 1) % m_max;
			m_count = m_max;
			m_vec[index] = item;
		}
		else
		{
			m_vec.push_back(item);
		}
		return adjusted;
	}

	size_t Size() const
	{
		return m_count;
	}

	size_t Capacity() const
	{
		return m_max;
	}

	void Clear()
	{
		m_vec.clear();
		m_start = 0;
		m_count = 0;
	}

	T& operator[](size_t index)
	{
		if (index >= m_count)
		{
			throw std::out_of_range("Index out of range!");
		}
		auto realIndex = (m_start + index) % m_max;
		return m_vec[realIndex];
	}

private:
	std::vector<T> m_vec;
	size_t m_start = 0;
	size_t m_count = 0;
	size_t m_max = 0;
};
