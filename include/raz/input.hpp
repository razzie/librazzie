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

#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <tuple>
#include <type_traits>
#include "raz/hash.hpp"

#ifndef RAZ_INPUT_AXIS_DIMENSION
#define RAZ_INPUT_AXIS_DIMENSION 2
#endif

namespace raz
{
	struct Input
	{
		enum InputType : unsigned
		{
			Unknown =        (1 << 0),
			ButtonPressed =  (1 << 1),
			ButtonHold =     (1 << 2),
			ButtonReleased = (1 << 3),
			AxisChanged =    (1 << 4)
		};

		typedef std::array<float, RAZ_INPUT_AXIS_DIMENSION> AxisValue;

		Input() :
			type(InputType::Unknown), button(0), axis(0), device(0), action(0), handled(false)
		{
			axis_value.fill(0.f);
			axis_delta.fill(0.f);
		}

		InputType type;
		uint32_t button;
		uint32_t axis;
		AxisValue axis_value;
		AxisValue axis_delta;
		uint32_t device;
		uint32_t action;
		mutable bool handled;
	};

	class Action;
	typedef std::shared_ptr<Action> ActionPtr;

	class Action : public std::enable_shared_from_this<Action>
	{
	public:
		static ActionPtr createButtonAction(uint32_t button, Input::InputType mask = static_cast<Input::InputType>(Input::ButtonPressed | Input::ButtonHold))
		{
			class ButtonAction : public Action
			{
			public:
				ButtonAction(uint32_t button, Input::InputType mask) :
					m_button(button), m_mask(mask)
				{
				}

				virtual bool tryInput(const Input& input) const
				{
					return ((input.button == m_button) && (input.type & m_mask));
				}

			private:
				uint32_t m_button;
				Input::InputType m_mask;
			};

			return std::make_shared<ButtonAction>(button, mask);
		}

		static ActionPtr createAxisAction(uint32_t axis)
		{
			class AxisAction : public Action
			{
			public:
				AxisAction(uint32_t axis) :
					m_axis(axis)
				{
				}

				virtual bool tryInput(const Input& input) const
				{
					return (input.axis == m_axis);
				}

			private:
				uint32_t m_axis;
			};

			return std::make_shared<AxisAction>(axis);
		}

		virtual ~Action() = default;
		virtual bool tryInput(const Input&) const = 0;

		operator ActionPtr()
		{
			return shared_from_this();
		}
	};


	template<uint32_t ButtonCount, uint32_t AxisCount, uint32_t ID = 0>
	class InputDevice
	{
	public:
		enum ButtonState : unsigned
		{
			Released = (1 << 0),
			Pressed =  (1 << 1),
			Hold =     (1 << 2)
		};

		struct ButtonPressed
		{
			typedef InputDevice Device;

			ButtonPressed(uint32_t button) : button(button)
			{
			}

			uint32_t button;
		};

		struct ButtonReleased
		{
			typedef InputDevice Device;

			ButtonReleased(uint32_t button) : button(button)
			{
			}

			uint32_t button;
		};

		struct AxisChanged
		{
			typedef InputDevice Device;

			AxisChanged(uint32_t axis, Input::AxisValue axis_value) :
				axis(axis), axis_value(axis_value)
			{
			}

			uint32_t axis;
			Input::AxisValue axis_value;
		};

		InputDevice()
		{
			for (auto& axis_value : m_axis_values)
				axis_value.fill(0.f);
		}

		static constexpr uint32_t getID()
		{
			return ID;
		}

		ButtonState getButtonState(uint32_t button) const
		{
			return m_btn_states[button];
		}

		bool isButtonDown(uint32_t button) const
		{
			unsigned mask = ButtonState::Hold | ButtonState::Pressed;
			return (m_btn_states[button] & mask);
		}

		const Input::AxisValue& getAxisValue(uint32_t axis) const
		{
			m_axis_values[axis];
		}

		Input operator()(ButtonPressed event)
		{
			auto& state = m_btn_states[event.button];
			if (state == ButtonState::Pressed)
				state = ButtonState::Hold;
			else
				state = ButtonState::Pressed;

			Input input;
			input.type = Input::ButtonPressed;
			input.button = event.button;
			input.device = ID;
			return input;
		}

		Input operator()(ButtonReleased event)
		{
			auto& state = m_btn_states[event.button];
			state = ButtonState::Released;

			Input input;
			input.type = Input::ButtonReleased;
			input.button = event.button;
			input.device = ID;
			return input;
		}

		Input operator()(AxisChanged event)
		{
			auto& axis_value = m_axis_values[event.axis];
			auto old_axis_value = axis_value;
			axis_value = event.axis_value;

			Input input;
			input.type = Input::AxisChanged;
			input.axis = event.axis;
			input.axis_value = event.axis;
			input.axis_delta = event.axis_value;
			for (int i = 0; i < input.axis_delta.size(); ++i) input.axis_delta[i] -= old_axis_value[i];
			input.device = ID;
			return input;
		}

	private:
		std::conditional_t<(ButtonCount > 256),
			typename std::map<uint32_t, ButtonState>,
			typename std::array<ButtonState, ButtonCount>> m_btn_states;
		std::array<Input::AxisValue, AxisCount> m_axis_values;
	};

	typedef InputDevice<256, 0> CharKeyboard;
	typedef InputDevice<~0u, 0> Keyboard;
	typedef InputDevice<3, 2> Mouse; // three buttons + one XY axis and a mouse wheel one
	template<uint32_t ID> using GamePad = typename InputDevice<12, 4, ID>;


	template<class... InputDevices>
	class ActionMap
	{
	public:
		void bind(uint32_t action_id, ActionPtr action_ptr)
		{
			m_actions[action_id] = action_ptr;
		}

		void unbind(uint32_t action_id)
		{
			m_actions.erase(action_id);
		}

		template<class InputEvent, class... Handlers>
		void operator()(InputEvent event, Handlers&... handlers)
		{
			auto& device = std::get<typename InputEvent::Device>(m_devices);
			auto input = device(event);
			tryActions(input, handlers...);
		}

	private:
		std::tuple<InputDevices...> m_devices;
		std::map<uint32_t, ActionPtr> m_actions;

		template<class... Handlers>
		void tryActions(Input input, Handlers&... handlers)
		{
			for (auto& action : m_actions)
			{
				if (action.second->tryInput(input))
				{
					input.action = action.first;
					invokeHandlers(input, handlers...);
				}
			}
		}

		template<class Handler0, class... Handlers>
		void invokeHandlers(const Input& input, Handler0& handler, Handlers&... handlers)
		{
			handler(input);
			if (!input.handled) invokeHandlers(input, handlers...);
		}

		template<class... Handlers>
		void invokeHandlers(const Input& input, Handlers&... handlers)
		{
		}
	};

	namespace literal
	{
		constexpr uint32_t operator"" _action(const char* action, size_t)
		{
			return (uint32_t)hash(action);
		}
	}
}

raz::ActionPtr operator&&(raz::ActionPtr action1, raz::ActionPtr action2)
{
	class AndAction : public raz::Action
	{
	public:
		AndAction(raz::ActionPtr action1, raz::ActionPtr action2) :
			m_action1(action1), m_action2(action2)
		{
		}

		virtual bool tryInput(const raz::Input& input) const
		{
			return (m_action1->tryInput(input) && m_action2->tryInput(input));
		}

	private:
		raz::ActionPtr m_action1;
		raz::ActionPtr m_action2;
	};

	return std::make_shared<AndAction>(action1, action2);
}

raz::ActionPtr operator||(raz::ActionPtr action1, raz::ActionPtr action2)
{
	class OrAction : public raz::Action
	{
	public:
		OrAction(raz::ActionPtr action1, raz::ActionPtr action2) :
			m_action1(action1), m_action2(action2)
		{
		}

		virtual bool tryInput(const raz::Input& input) const
		{
			return (m_action1->tryInput(input) || m_action2->tryInput(input));
		}

	private:
		raz::ActionPtr m_action1;
		raz::ActionPtr m_action2;
	};

	return std::make_shared<OrAction>(action1, action2);
}

raz::ActionPtr operator!(raz::ActionPtr action)
{
	class NotAction : public raz::Action
	{
	public:
		NotAction(raz::ActionPtr action) :
			m_action(action)
		{
		}

		virtual bool tryInput(const raz::Input& input) const
		{
			return (!m_action->tryInput(input));
		}

	private:
		raz::ActionPtr m_action;
	};

	return std::make_shared<NotAction>(action);
}
