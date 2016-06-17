#include <iostream>
#include <sstream>
#include "raz/stream.hpp"

struct Foo
{
};

int main()
{
	std::cout << std::boolalpha
		<< raz::HasStreamInserter<int>::value << "\n"
		<< raz::HasStreamInserter<Foo>::value << "\n"
		<< std::endl;

	std::stringstream ss;
	ss << raz::insert(123) << "\n" << raz::insert(Foo {});

	std::cout << ss.str() << std::endl;

	return 0;
}
