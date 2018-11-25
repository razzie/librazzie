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
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>
#include <sstream>

namespace raz
{
	class Logger
	{
	public:
		enum Output
		{
			StdOut,
			StdErr,
			StdLog,
			File
		};

		Logger(Output default_output = Output::StdOut) :
			m_start_time(std::chrono::system_clock::now()),
			m_default_output(default_output)
		{
		}

		template<class... Args>
		void operator()(std::string format, const Args&... args)
		{
			operator()(m_default_output, format, args...);
		}

		template<class... Args>
		void operator()(Output output, std::string format, const Args&... args)
		{
			std::array<std::string, sizeof...(Args)> str_args{ toString(args)... };

			std::lock_guard<std::mutex> guard(m_mutex);

			int curr_arg = 0;
			std::ostream& out = *getOutput(output);

			printTimeStamp(out);
			for (char c : format)
			{
				if (c == '%')
					out << str_args.at(curr_arg++);
				else
					out << c;
			}
			out << std::endl;
		}

	private:
		std::mutex m_mutex;
		std::chrono::time_point<std::chrono::system_clock> m_start_time;
		Output m_default_output;
		std::ofstream m_logfile;

		std::ostream* getOutput(Output output)
		{
			switch (output)
			{
			case Output::StdOut:
				return &std::cout;

			case Output::StdErr:
				return &std::cerr;

			case Output::StdLog:
				return &std::clog;

			case Output::File:
				if (!m_logfile.is_open())
					openLogFile();
				return &m_logfile;

			default:
				return nullptr;
			}
		}

		void printTimeStamp(std::ostream& out)
		{
			auto now = std::chrono::system_clock::now();
			auto time = std::chrono::system_clock::to_time_t(now);
			struct tm time_info;

#ifdef _WIN32
			localtime_s(&time_info, &time);
#elif
			time_info = *std::localtime(&in_time_t);
#endif

			out << std::put_time(&time_info, "[%H:%M:%S] ");
		}

		void openLogFile()
		{
			char filename[24];
			auto time = std::chrono::system_clock::to_time_t(m_start_time);
			struct tm time_info;

#ifdef _WIN32
			localtime_s(&time_info, &time);
#elif
			time_info = *std::localtime(&in_time_t);
#endif

			std::strftime(filename, sizeof(filename), "log_%Y%m%d_%H%M%S.txt", &time_info);
			m_logfile.open(filename, std::ios_base::out);
		}

		static inline std::string toString(std::string str)
		{
			return str;
		}

		template<class T>
		static inline std::string toString(const T& t)
		{
			std::stringstream ss;
			ss << t;
			return ss.str();
		}
	};
}
