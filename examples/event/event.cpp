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

//#define USE_EVENT_DISPATCHER

using namespace raz::literal; // for the _event literal operator

typedef raz::Event<"foo"_event, std::string, int> FooEvent;
typedef raz::Event<"bar"_event, float> BarEvent;

// this example shows two possible approaches:
#ifdef USE_EVENT_DISPATCHER // either you use EventDispatcher
FooEvent::CallbackSystem foo_callbacks;
BarEvent::CallbackSystem bar_callbacks;
raz::EventDispatcher<FooEvent, BarEvent> event_dispatcher(foo_callbacks, bar_callbacks);
#else // or EventSystem
raz::EventSystem<FooEvent, BarEvent> event_system;
#endif

struct FooEventReceiver : public FooEvent::Callback
{
	FooEventReceiver() :
#ifdef USE_EVENT_DISPATCHER
		FooEvent::Callback(foo_callbacks, &FooEventReceiver::receive)
#else
		FooEvent::Callback(event_system.getSystemByCallback(this), &FooEventReceiver::receive)
#endif
	{
	}

	void receive(const FooEvent& e)
	{
		std::cout << e.get<0>() << " - " << e.get<1>() << std::endl;
	}
};

int main()
{
	FooEventReceiver r;
	FooEvent e("razzie", 99);
#ifdef USE_EVENT_DISPATCHER
	event_dispatcher.handle(e);
#else
	event_system.handle(e);
#endif

	return 0;
}
