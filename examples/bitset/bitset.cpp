//#define USE_CONTAINER_EXAMPLE
#ifdef USE_CONTAINER_EXAMPLE
#include <array>
#include <iterator>
#include <type_traits>
#endif
#include <iostream>
#include "raz/bitset.hpp"

#ifdef USE_CONTAINER_EXAMPLE
template<class T, size_t N>
class Container
{
	static_assert(std::is_pod<T>::value, "T should be plain-old-data type");

public:
	class Iterator : public std::iterator<std::input_iterator_tag, const T>
	{
	public:
		Iterator(const Iterator& other) :
			m_data(other.m_data), m_internal_iterator(other.m_internal_iterator)
		{
		}

		Iterator& operator=(const Iterator& other)
		{
			m_data = other.m_data;
			m_internal_iterator = other.m_internal_iterator;
			return *this;
		}

		Iterator& operator++()
		{
			++m_internal_iterator;
			return *this;
		}

		Iterator operator++(int)
		{
			Iterator retval = *this;
			++(*this);
			return retval;
		}

		bool operator==(const Iterator& other) const
		{
			return (m_data == other.m_data && m_internal_iterator == other.m_internal_iterator);
		}

		bool operator!=(const Iterator& other) const
		{
			return !(*this == other);
		}

		reference operator*() const
		{
			return m_data->at(*m_internal_iterator);
		}

	private:
		const std::array<T, N>* m_data;
		typename raz::Bitset<N>::TrueBitIterator m_internal_iterator;

		friend class Container;

		Iterator(const std::array<T, N>* data, typename raz::Bitset<N>::TrueBitIterator internal_iterator) :
			m_data(data), m_internal_iterator(internal_iterator)
		{
		}
	};

	Container() = default;
	Container(const Container&) = delete;

	bool add(const T& t)
	{
		auto it = m_elements_bits.falsebits().begin();

		if (it == m_elements_bits.falsebits().end())
		{
			return false; // container is full
		}

		size_t pos = *it;
		m_elements_bits.set(pos);
		m_elements[pos] = t;
		return true;
	}

	void remove(const T& t)
	{
		for (auto it : m_elements_bits.truebits())
		{
			if (m_elements[it] == t)
			{
				m_elements_bits.unset(it);
				return;
			}
		}
	}

	size_t count() const
	{
		return m_elements_bits.truebits().count();
	}

	Iterator begin() const
	{
		return Iterator(&m_elements, m_elements_bits.truebits().begin());
	}

	Iterator end() const
	{
		return Iterator(&m_elements, m_elements_bits.truebits().end());
	}

private:
	std::array<T, N> m_elements;
	raz::Bitset<N> m_elements_bits;
};
#endif

int main()
{
	raz::Bitset<32> bitset; // by default all 32 bits are set to false

	bitset.set(1);
	bitset.set(6);
	bitset.set(22);
	bitset.set(23);
	bitset.set(24);

	std::cout << "The following bits are set to true:" << std::endl;
	for (size_t position : bitset.truebits())
	{
		std::cout << position << ", ";
	}
	std::cout << std::endl;

	std::cout << "The following bits are set to false:" << std::endl;
	for (size_t position : bitset.falsebits())
	{
		std::cout << position << ", ";
	}
	std::cout << std::endl;


#ifdef USE_CONTAINER_EXAMPLE
	Container<int, 32> container;

	container.add(7);
	container.add(99);
	container.add(123);
	container.add(-50);
	container.remove(7);

	std::cout << "The following elements are in the container:" << std::endl;
	for (int element : container)
	{
		std::cout << element << ", ";
	}
	std::cout << std::endl;
#endif

	return 0;
}
