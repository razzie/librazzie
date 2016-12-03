#include <iostream>
#include <string>
#include "raz/event.hpp"

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
