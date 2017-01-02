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
#include "raz/callback.hpp"

struct Foo
{
	int value;
};

struct Bar
{
	float value;
};

raz::CallbackSystem<Foo> foo_callbacks;
raz::CallbackSystem<Bar> bar_callbacks;

struct FooCallback : public raz::Callback<Foo>
{
	FooCallback() :
		Callback<Foo>(foo_callbacks)
	{
	}

	virtual void handle(const Foo& foo) // inherited from Callback<Foo>
	{
		std::cout << "FooCallback - foo: " << foo.value << std::endl;
	}
};

struct FooBarCallback : public raz::Callback<Foo>, public raz::Callback<Bar>
{
	FooBarCallback() :
		Callback<Foo>(foo_callbacks),
		Callback<Bar>(bar_callbacks)
	{
	}

	virtual void handle(const Foo& foo) // inherited from Callback<Foo>
	{
		std::cout << "FooBarCallback - foo: " << foo.value << std::endl;
	}

	virtual void handle(const Bar& bar) // inherited from Callback<Bar>
	{
		std::cout << "FooBarCallback - bar: " << bar.value << std::endl;
	}
};

int main()
{
	FooCallback foo;
	FooBarCallback foobar;

	foo_callbacks.handle(Foo{ 123 });
	bar_callbacks.handle(Bar{ 1.23f });

	return 0;
}
