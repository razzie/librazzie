#include <iostream>
#include <string>
#include "raz/serialization.hpp"

template<size_t SIZE>
class Buffer
{
public:
	Buffer() :
		m_mode(raz::SerializationMode::SERIALIZE),
		m_data_len(0),
		m_data_pos(0)
	{
	}

	raz::SerializationMode getMode() const
	{
		return m_mode;
	}

	void setMode(raz::SerializationMode mode)
	{
		m_mode = mode;
	}

	size_t write(const char* ptr, size_t len)
	{
		if (SIZE - m_data_len < len)
			len = SIZE - m_data_len;

		std::memcpy(&m_data[m_data_len], ptr, len);
		m_data_len += len;
		return len;
	}

	size_t read(char* ptr, size_t len)
	{
		if (m_data_len - m_data_pos < len)
			len = m_data_len - m_data_pos;

		std::memcpy(ptr, &m_data[m_data_pos], len);
		m_data_pos += len;
		return len;
	}

private:
	char m_data[SIZE];
	raz::SerializationMode m_mode;
	size_t m_data_len;
	size_t m_data_pos;
};

struct Foo
{
	std::string user;
	int age;

	template<class Serializer>
	raz::EnableSerializer<Serializer> operator()(Serializer& serializer)
	{
		serializer(user)(age);
	}
};

int main()
{
	raz::Serializer<Buffer<1024>> serializer;
	Foo foo_src = { "razzie", 99 };
	Foo foo_dest;

	serializer.setMode(raz::SerializationMode::SERIALIZE);
	serializer(foo_src);

	serializer.setMode(raz::SerializationMode::DESERIALIZE);
	serializer(foo_dest);

	std::cout << foo_dest.user << " - " << foo_dest.age << std::endl;

	return 0;
}
