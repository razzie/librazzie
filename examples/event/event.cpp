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
#include "raz/event.hpp"

struct XEvent
{
	int x;
};

struct YEvent
{
	float y;
};

struct ZEvent
{
	bool z;
};

struct XYZEventReceiver : /* public */ raz::EventReceiver<XYZEventReceiver>
{
	void operator()(const XEvent& e)
	{
		std::cout << "XEvent: " << e.x << std::endl;
	}

	void operator()(const YEvent& e)
	{
		std::cout << "YEvent: " << e.y << std::endl;
	}

	void operator()(const ZEvent& e)
	{
		std::cout << "ZEvent: " << e.z << std::endl;
	}
};

int main()
{
	raz::TaskManager taskmgr;
	raz::EventDispatcher dispatcher(&taskmgr);

	auto receiver = std::make_shared<XYZEventReceiver>();
	receiver->bind<XEvent, YEvent, ZEvent>(dispatcher);

	XEvent e{ 123 };
	dispatcher(e);

	return 0;
}
