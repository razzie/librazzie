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

namespace raz
{
	class IInputDevice;

	struct Input
	{
		enum InputType : unsigned
		{
			Unknown =        (1 << 0),
			ButtonPressed =  (1 << 1),
			ButtonHold =     (1 << 2),
			ButtonReleased = (1 << 3),
			ChannelChanged =    (1 << 4)
		};

		struct ChannelValue
		{
			ChannelValue() :
				primary(0.f), secondary(0.f)
			{
			}

			ChannelValue(float value) :
				primary(value), secondary(0.f)
			{
			}

			ChannelValue(float primary, float secondary) :
				primary(primary), secondary(secondary)
			{
			}

			operator float() const
			{
				return primary;
			}

			ChannelValue operator-(const ChannelValue& other)
			{
				return ChannelValue(primary - other.primary, secondary - other.secondary);
			}

			// a channel should usually have a single value, but there are cases
			// when we want them bundled (like mouse XY)
			float primary;
			float secondary;
		};

		Input() :
			type(InputType::Unknown), action(0),
			button(0),
			channel(0), channel_value(0.f), channel_delta(0.f),
			device(nullptr),
			handled(false)
		{
		}

		InputType type;
		uint32_t action;
		uint32_t button;
		uint32_t channel;
		ChannelValue channel_value;
		ChannelValue channel_delta;
		IInputDevice* device;
		mutable bool handled;
	};

	class IInputDevice
	{
	public:
		enum ButtonState : unsigned
		{
			Released = (1 << 0),
			Pressed =  (1 << 1),
			Hold =     (1 << 2)
		};

		typedef Input::ChannelValue(*ChannelCharacteristics)(const Input::ChannelValue&);

		virtual uint32_t getID() const = 0;
		virtual ButtonState getButtonState(uint32_t button) const = 0;
		virtual bool isButtonDown(uint32_t button) const = 0;
		virtual const Input::ChannelValue& getChannelValue(uint32_t channel) const = 0;
		virtual void setChannelCharacteristics(uint32_t channel, ChannelCharacteristics) = 0;
	};

	template<uint32_t DeviceID, uint32_t ButtonCount, uint32_t ChannelCount>
	class InputDevice : public IInputDevice
	{
	public:
		enum : uint32_t { ID = DeviceID };

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

		struct ChannelChanged
		{
			typedef InputDevice Device;

			ChannelChanged(uint32_t channel, Input::ChannelValue channel_value) :
				channel(channel), channel_value(channel_value)
			{
			}

			uint32_t channel;
			Input::ChannelValue channel_value;
		};

		InputDevice()
		{
		}

		virtual uint32_t getID() const
		{
			return DeviceID;
		}

		virtual ButtonState getButtonState(uint32_t button) const
		{
			return m_btn_states[button];
		}

		virtual bool isButtonDown(uint32_t button) const
		{
			unsigned mask = ButtonState::Hold | ButtonState::Pressed;
			return (m_btn_states[button] & mask);
		}

		virtual const Input::ChannelValue& getChannelValue(uint32_t channel) const
		{
			return m_channel_values[channel];
		}

		virtual void setChannelCharacteristics(uint32_t channel, ChannelCharacteristics fn)
		{
			m_channel_characteristics[channel] = fn;
		}

		Input operator()(ButtonPressed event)
		{
			auto& state = m_btn_states[event.button];
			if (state == ButtonState::Pressed || state == ButtonState::Hold)
				state = ButtonState::Hold;
			else
				state = ButtonState::Pressed;

			Input input;
			input.type = Input::ButtonPressed;
			input.button = event.button;
			input.device = this;
			return input;
		}

		Input operator()(ButtonReleased event)
		{
			auto& state = m_btn_states[event.button];
			state = ButtonState::Released;

			Input input;
			input.type = Input::ButtonReleased;
			input.button = event.button;
			input.device = this;
			return input;
		}

		Input operator()(ChannelChanged event)
		{
			auto& channel_value = m_channel_values[event.channel];
			auto characteristics = m_channel_characteristics[event.channel];
			auto old_channel_value = channel_value;
			channel_value = event.channel_value;

			Input input;
			input.type = Input::ChannelChanged;
			input.channel = event.channel;
			input.channel_value = (characteristics) ? characteristics(event.channel_value) : event.channel_value;
			input.channel_delta = event.channel_value - old_channel_value;
			input.device = this;
			return input;
		}

	private:
		mutable std::conditional_t<(ButtonCount > 256),
			std::map<uint32_t, ButtonState>,
			std::array<ButtonState, ButtonCount>> m_btn_states;
		std::array<Input::ChannelValue, ChannelCount> m_channel_values;
		std::array<ChannelCharacteristics, ChannelCount> m_channel_characteristics;
	};

	typedef InputDevice<hash32("Keyboard"), ~0u, 0> Keyboard;
	typedef InputDevice<hash32("Mouse"), 3, 2> Mouse; // three buttons + an XY bundled channel and a mouse wheel channel
	template<uint32_t ID> using GamePad = typename InputDevice<ID, 12, 4>;


	class Action;
	typedef std::shared_ptr<Action> ActionPtr;

	class Action : public std::enable_shared_from_this<Action>
	{
	public:
		template<class Device>
		static ActionPtr button(uint32_t button, Input::InputType mask = static_cast<Input::InputType>(Input::ButtonPressed | Input::ButtonHold))
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
					if (input.device->getID() == Device::ID)
					{
						if ((input.button == m_button) && (input.type & m_mask))
							return true;
						else if (tryDeviceInput(input.device))
							return true;
					}

					return false;
				}

			private:
				uint32_t m_button;
				Input::InputType m_mask;

				bool tryDeviceInput(const IInputDevice* device) const
				{
					auto btn_state = device->getButtonState(m_button);
					auto input_type = mapButtonStateToInputType(btn_state);

					return (input_type & m_mask);
				}

				static Input::InputType mapButtonStateToInputType(IInputDevice::ButtonState btn_state)
				{
					switch (btn_state)
					{
					case IInputDevice::Pressed:
						return Input::ButtonPressed;
					case IInputDevice::Hold:
						return Input::ButtonHold;
					case IInputDevice::Released:
						return Input::ButtonReleased;
					default:
						return Input::Unknown;
					}
				}
			};

			return std::make_shared<ButtonAction>(button, mask);
		}

		template<class Device>
		static ActionPtr channel(uint32_t channel)
		{
			class ChannelAction : public Action
			{
			public:
				ChannelAction(uint32_t channel) :
					m_channel(channel)
				{
				}

				virtual bool tryInput(const Input& input) const
				{
					return ((input.device->getID() == Device::ID) && (input.channel == m_channel));
				}

			private:
				uint32_t m_channel;
			};

			return std::make_shared<ChannelAction>(channel);
		}

		virtual ~Action() = default;
		virtual bool tryInput(const Input&) const = 0;

		operator ActionPtr()
		{
			return shared_from_this();
		}
	};

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

		template<class Device>
		Device& getInputDevice()
		{
			return std::get<Device>(m_devices);
		}

		template<class Device>
		const Device& getInputDevice() const
		{
			return std::get<Device>(m_devices);
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
			return hash32(action);
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
