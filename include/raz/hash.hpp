/*
Copyright (C) G�bor "Razzie" G�rzs�ny

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
#pragma warning(disable: 4307)

#include <cstdint>

namespace raz
{
	/*
	Modified, recursive version of DJB2 hash function
	https://en.wikipedia.org/wiki/DJB2
	*/

	inline constexpr uint64_t hash(const char* str, const uint64_t h = 5381)
	{
		return (str[0] == 0) ? h : hash(&str[1], h * 33 + str[0]);
	}

	inline constexpr uint32_t hash32(const char* str)
	{
		return static_cast<uint32_t>(hash(str));
	}

	template<class T>
	inline constexpr uint64_t hash()
	{
#ifdef _MSC_VER
		return raz::hash(__FUNCSIG__);
#else
		return raz::hash(__PRETTY_FUNCTION__);
#endif
	}

	template<class T>
	inline constexpr uint32_t hash32()
	{
		return static_cast<uint32_t>(hash<T>());
	}
}
