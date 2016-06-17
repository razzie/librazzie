#include <iostream>
#include "raz/callback.hpp"

struct Foo
{
	int i;
};

raz::CallbackSystem<Foo> foo_subscribers;

struct FooReceiver : public raz::Callback<Foo>
{
	FooReceiver() : Callback<Foo>(foo_subscribers, &FooReceiver::receive)
	{
	}

	void receive(const Foo& foo)
	{
		std::cout << foo.i << std::endl;
	}
};

int main()
{
	FooReceiver r;
	Foo f = { 123 };

	foo_subscribers.handle(f);

	return 0;
}
