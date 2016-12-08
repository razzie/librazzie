/*
Copyright (C) 2016 - G�bor "Razzie" G�rzs�ny

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
#include "raz/bitset.hpp"

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

	return 0;
}
