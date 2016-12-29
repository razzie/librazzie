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
#include <string>
#include "raz/event.hpp"

using namespace raz::literal; // for the _event literal operator

typedef raz::Event<"foo"_event, std::string, int> FooEvent;
typedef raz::Event<"bar"_event, float> BarEvent;

struct FooEventReceiver : public FooEvent::Callback
{
	FooEventReceiver(FooEvent::CallbackSystem& system) :
		FooEvent::Callback(system, &FooEventReceiver::receive)
	{
	}

	void receive(const FooEvent& e)
	{
		std::cout << e.get<0>() << " - " << e.get<1>() << std::endl;
	}
};

void example01()
{
	FooEvent::CallbackSystem foo_callbacks;
	FooEventReceiver r(foo_callbacks);
	foo_callbacks.handle(FooEvent("razzie", 99));
}

void example02()
{
	FooEvent::CallbackSystem foo_callbacks;
	BarEvent::CallbackSystem bar_callbacks;
	raz::EventDispatcher<FooEvent, BarEvent> event_handler(foo_callbacks, bar_callbacks);

	FooEventReceiver r(foo_callbacks);

	FooEvent e("razzie", 99);
	event_handler.handle(e);
}

void example03()
{
	raz::EventSystem<FooEvent, BarEvent> event_handler;
	raz::EventQueueSystem<FooEvent, BarEvent> event_queue;

	FooEventReceiver r(event_handler.access<FooEventReceiver>());

	event_queue.enqueue<"foo"_event>("razzie", 99);
	event_queue.dequeue(event_handler);
}

int main()
{
	example01();
	example02();
	example03();

	return 0;
}
