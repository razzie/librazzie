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
#include <stdexcept>
#include "raz/thread.hpp"

class Loopable
{
public:
	void operator()()
	{
		std::cout << "loop" << std::endl;
	}
};

class Worker
{
	int base_value;

public:
	Worker(int base_value) : base_value(base_value)
	{
	}

	void operator()(int value)
	{
		std::cout << (base_value * value) << std::endl;
	}
};

class WorkerWithException
{
public:
	void operator()(bool throw_exception)
	{
		if (throw_exception)
			throw std::runtime_error("this is an exception");
	}

	void operator()(std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
};

void example01()
{
	raz::Thread<Loopable> thread;

	thread.start();

	std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

void example02()
{
	raz::Thread<Worker> thread;

	for (int i = 0; i < 3; ++i)
	{
		thread.start(111);
		thread(1);
		thread(2);
		thread(3);

		std::this_thread::sleep_for(std::chrono::milliseconds(200));
		thread.stop();
	}
}

void example03()
{
	raz::Thread<WorkerWithException> thread;

	thread.start();
	thread(false);
	thread(true);

	std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

void example04()
{
	raz::TaskManager taskmgr;
	taskmgr([] { std::cout << "hello from task" << std::endl; });
}

int main()
{
	example01();
	example02();
	example03();
	example04();

	return 0;
}
