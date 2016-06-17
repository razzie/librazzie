#include <iostream>
#include <string>
#include "raz/event.hpp"

using namespace raz::literal; // for the _event literal operator

typedef raz::Event<"foo"_event, std::string, int> FooEvent;
typedef raz::Event<"bar"_event, float> BarEvent;

FooEvent::CallbackSystem foo_callbacks;
BarEvent::CallbackSystem bar_callbacks;
raz::EventDispatcher<FooEvent, BarEvent> event_dispatcher(foo_callbacks, bar_callbacks);

struct FooEventReceiver : public FooEvent::Callback
{
	FooEventReceiver() : FooEvent::Callback(foo_callbacks, &FooEventReceiver::receive)
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
	event_dispatcher.handle(e);

	return 0;
}
