/*
Copyright (C) 2016 - Gábor "Razzie" Görzsöny

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
#include "raz/random.hpp"

int main()
{
	uint64_t seed = 12345;
	raz::RandomGenerator gen(seed);
	raz::RandomDistributor<raz::RandomGenerator> random(gen);

	std::cout << "Generating some random integers in range [1..100]:" << std::endl;
	for (int i = 0; i < 10; ++i)
	{
		std::cout << random(1, 100) << " ";
	}
	std::cout << std::endl;

	std::cout << "Generating some random floats in range [0..1]:" << std::endl;
	for (int i = 0; i < 10; ++i)
	{
		std::cout << random(0.f, 1.f) << " ";
	}
	std::cout << std::endl;

	return 0;
}
