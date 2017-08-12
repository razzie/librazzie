/*
Copyright (C) 2016 - Gábor "Razzie" Görzsöny

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

#include <cstdint>

#pragma once

namespace raz
{
	struct Color
	{
		Color() : r(0), g(0), b(0), a(255)
		{
		}

		Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : r(r), g(g), b(b), a(a)
		{
		}

		uint8_t r, g, b, a;
	};

	class ColorTable
	{
	public:
		ColorTable() :
			m_table {
				Color(255, 0, 0),
				Color(0, 255, 0),
				Color(0, 0, 255),
				Color(255, 255, 0),
				Color(255, 0, 255),
				Color(0, 255, 255)
			}
		{
		}

		Color operator[](size_t n) const
		{
			Color base = m_table[n % 6];

			for (size_t iter = n / 6; iter > 0; iter /= 6)
			{
				Color sub = m_table[iter % 6];
				sub.r /= (uint8_t)iter + 1;
				sub.g /= (uint8_t)iter + 1;
				sub.b /= (uint8_t)iter + 1;

				base.r -= sub.r;
				base.g -= sub.g;
				base.b -= sub.b;
			}

			return base;
		}

	private:
		Color m_table[6];
	};
}
