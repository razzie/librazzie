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
#include <conio.h>
#include "raz/input.hpp"

using namespace raz::literal;

struct ActionHandler
{
	void operator()(const raz::Input& input)
	{
		switch (input.action)
		{
		case "ab"_action:
			std::cout << "\"ab\" action received" << std::endl;
			break;

		default:
			std::cout << "unknown action" << std::endl;
			break;
		}
	}
};

int main()
{
	raz::ActionMap<raz::Keyboard, raz::Mouse> action_map;
	ActionHandler handler;

	auto action = raz::Action::createButtonAction('a') || raz::Action::createButtonAction('b');
	action_map.bind("ab"_action, action);

	int input;
	do
	{
		input = _getch();
		action_map(raz::Keyboard::ButtonPressed(input), handler);

	} while (input != '\r');

	return 0;
}
