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

#include <functional>
#include <iterator>
#include <map>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

namespace raz
{
	template<class CharT>
	class CommandLineParser
	{
	public:
		using String = std::basic_string<CharT>;
		using Stream = std::basic_istream<CharT>;
		using StreamIterator = std::istream_iterator<String, CharT>;

		template<class F>
		void addCommand(String cmd, F func)
		{
			m_commands.emplace(cmd, func);
		}

		void operator()(int argc, CharT** argv)
		{
			parse(argv, argv + argc);
		}

		void operator()(Stream& stream)
		{
			parse(StreamIterator(stream), StreamIterator());
		}

	private:
		class Cmd
		{
		public:
			template<class R, class... Args>
			Cmd(R(*func)(Args...)) :
				m_func(makeFunc(func)),
				m_args(sizeof...(Args))
			{
			}

			template<class F>
			Cmd(F func_obj) :
				Cmd(&F::operator(), func_obj)
			{
			}

			template<class Iter>
			void exec(Iter& it, Iter end)
			{
				m_func(getCmdArgs(it, end));
			}

		private:
			using CmdArgs = std::vector<String>;
			using Func = std::function<void(const CmdArgs&)>;

			Func m_func;
			size_t m_args;

			template<class R, class F, class... Args>
			Cmd(R(F::*func)(Args...) const, F func_obj) :
				m_func(makeFunc(func, func_obj)),
				m_args(sizeof...(Args))
			{
			}

			template<class Iter>
			CmdArgs getCmdArgs(Iter& it, Iter end) const
			{
				std::vector<String> args;
				args.reserve(m_args);

				for (; it != end && args.size() < m_args; ++it)
				{
					args.push_back(*it);
				}

				return args;
			}

			template<class Arg0, class Arg1, class... Args>
			static std::tuple<Arg0, Arg1, Args...> convertArgs(const CmdArgs& args, size_t pos = 0)
			{
				auto a = convertArgs<Arg0>(args, pos);
				auto b = convertArgs<Arg1, Args...>(args, pos + 1);
				return std::tuple_cat(a, b);
			}

			template<class Arg0>
			static std::tuple<Arg0> convertArgs(const CmdArgs& args, size_t pos = 0)
			{
				std::stringstream ss(args.at(pos));
				Arg0 arg0;
				ss >> arg0;
				return std::forward_as_tuple(arg0);
			}

			template<class F, class Tuple, std::size_t... I>
			static constexpr decltype(auto) apply(F&& f, Tuple&& t, std::index_sequence<I...>)
			{
				return std::invoke(std::forward<F>(f), std::get<I>(std::forward<Tuple>(t))...);
			}

			template<class R>
			static Func makeFunc(R(*func)())
			{
				return [func](const CmdArgs&)
				{
					func();
				};
			}

			template<class R, class F>
			static Func makeFunc(R(F::*func)() const, F func_obj)
			{
				return [func, func_obj](const CmdArgs& args)
				{
					std::invoke(func, &func_obj);
				};
			}

			template<class R, class... Args>
			static Func makeFunc(R(*func)(Args...))
			{
				return [func](const CmdArgs& args)
				{
					auto tup = convertArgs<Args...>(args);
					apply(func, tup, std::make_index_sequence<sizeof...(Args)>{});
				};
			}

			template<class R, class F, class... Args>
			static Func makeFunc(R(F::*func)(Args...) const, F func_obj)
			{
				return [func, func_obj](const CmdArgs& args)
				{
					auto tup = std::tuple_cat(std::make_tuple(&func_obj), convertArgs<Args...>(args));
					apply(func, tup, std::make_index_sequence<sizeof...(Args) + 1>{});
				};
			}
		};

		std::map<String, Cmd> m_commands;

		template<class Iter>
		void parse(Iter begin, Iter end)
		{
			for (auto it = begin; it != end; )
			{
				auto cmd = m_commands.find(*it);
				if (cmd == m_commands.end())
				{
					++it;
					continue; // skipping unknown command
				}
				else
				{
					++it;
					cmd->second.exec(it, end);
				}
			}
		}
	};

	typedef CommandLineParser<char> CmdlineParser;
	typedef CommandLineParser<wchar_t> WCmdlineParser;
}
