/*
Copyright (C) Gábor "Razzie" Görzsöny

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE
*/

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
	enum Bar
	{
		BAR1 = 1,
		BAR2 = 2,
		BAR3 = 3
	};

	std::string str;
	std::tuple<int, float> tup;
	Bar bar;

	template<class Serializer>
	raz::EnableSerializer<Serializer> operator()(Serializer& serializer)
	{
		serializer(str)(tup)(bar);
	}

	friend std::ostream& operator<<(std::ostream& o, const Foo& foo)
	{
		o << foo.str << " - " << std::get<0>(foo.tup) << " - " << std::get<1>(foo.tup) << " - " << foo.bar;
		return o;
	}
};

struct FooDeserializer : public raz::Deserializer<Foo>
{
	void handle(Foo&& foo)
	{
		std::cout << foo << std::endl;
	}
};

void example01()
{
	raz::Serializer<Buffer<1024>> serializer;
	Foo foo_src { "razzie", std::make_tuple( 100, 1.5f ), Foo::Bar::BAR1 };
	Foo foo_dest;

	serializer.setMode(raz::SerializationMode::SERIALIZE);
	serializer(foo_src);

	serializer.setMode(raz::SerializationMode::DESERIALIZE);
	serializer(foo_dest);

	std::cout << foo_dest << std::endl;
}

void example02()
{
	raz::Serializer<Buffer<1024>> serializer;
	Foo foo{ "razzie", std::make_tuple(100, 1.5f), Foo::Bar::BAR1 };

	serializer.setMode(raz::SerializationMode::SERIALIZE);
	serializer(foo);

	FooDeserializer defoo;
	defoo(raz::hash32<Foo>(), serializer);
}

int main()
{
	example01();
	example02();

	return 0;
}
