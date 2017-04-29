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
#include "raz/thread.hpp"

struct Loopable
{
	void operator()()
	{
		std::cout << "loop" << std::endl;
	}
};

struct Worker
{
	void operator()(int a, int b)
	{
		std::cout << (a + b) << std::endl;
	}
};

void example01()
{
	raz::Thread<Loopable> thread;
	std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

void example02()
{
	raz::Thread<Worker> thread;

	thread(1, 2);
	std::this_thread::sleep_for(std::chrono::milliseconds(200));

	thread(2, 3);
	std::this_thread::sleep_for(std::chrono::milliseconds(200));

	thread(3, 4);
	std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

int main()
{
	example01();
	example02();

	return 0;
}
