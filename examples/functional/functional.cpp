#include <iostream>
#include "raz/functional.hpp"

struct Foo
{
	int a;

	int bar(int b)
	{
		return a + b;
	}
};

int main()
{
	Foo f = { 123 };
	auto foobar = raz::bind(&Foo::bar, &f);

	std::cout << foobar(1) << std::endl;

	return 0;
}
