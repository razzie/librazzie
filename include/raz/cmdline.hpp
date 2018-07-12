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

#include <iterator>
#include <map>
#include <string>
#include "raz/functional.hpp"
#include "raz/stream.hpp"

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
			parseAll(argv, argv + argc);
		}

		void operator()(Stream& stream)
		{
			parseAll(StreamIterator(stream), StreamIterator());
		}

	private:
		class Cmd
		{
		public:
			template<class F>
			Cmd(F func)
			{

			}

			template<class R, class... Args>
			Cmd(R(*func)(Args...))
			{

			}

			template<class Iter>
			void exec(Iter& it, Iter end)
			{

			}

		private:
		};

		std::map<String, Cmd> m_commands;

		template<class Iter>
		void parseAll(Iter begin, Iter end)
		{
			for (auto it = begin; it != end; ++it)
			{
				parseSingle(it, end);
			}
		}

		template<class Iter>
		void parseSingle(Iter& it, Iter end)
		{
			auto cmd = m_commands.find(*it);
			if (cmd == m_commands.end())
				return; // skipping unknown command

			cmd->exec(it, end);
		}
	};

	typedef CommandLineParser<char> CmdlineParser;
	typedef CommandLineParser<wchar_t> WCmdlineParser;
}
