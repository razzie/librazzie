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

#include <iosfwd>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace raz
{
	template<class T>
	class HasStreamInserter
	{
		template<class U>
		static constexpr auto test() -> decltype(bool(std::declval<std::ostream&>() << std::declval<const U&>()))
		{
			return true;
		}

		template<class U>
		static constexpr bool test(...)
		{
			return false;
		}

	public:
		constexpr static bool value = test<T>();
	};

	template<class T>
	class HasStreamExtractor
	{
		template<class U>
		static constexpr auto test() -> decltype(bool(std::declval<std::ostream&>() >> std::declval<U&>()))
		{
			return true;
		}

		template<class U>
		static constexpr bool test(...)
		{
			return false;
		}

	public:
		constexpr static bool value = test<T>();
	};

	struct __FallbackType
	{
		template<class T>
		__FallbackType(const T&) : type(&typeid(T))
		{
		}

		friend std::ostream& operator<< (std::ostream& os, const __FallbackType& t)
		{
			os << t.type->name();
			return os;
		}

		friend std::istream& operator>> (std::istream& is, __FallbackType&)
		{
			return is;
		}

		const std::type_info* type;
	};

	template<class T>
	std::conditional_t<HasStreamInserter<T>::value, const T&, __FallbackType> insert(const T& t)
	{
		return { t };
	}

	template<class T>
	std::conditional_t<HasStreamExtractor<T>::value, T&, __FallbackType> extract(T& t)
	{
		return { t };
	}


	template<class To, class From>
	To lexical_cast(const From& from)
	{
		To to;
		std::sstream ss;
		ss << from;
		ss >> to;
		return to;
	}


	template<class T>
	class StreamManipulator
	{
	public:
		typedef std::ios&(*Manipulator)(std::ios&, T);

		explicit StreamManipulator(Manipulator m, T t) :
			m_manip(m), m_value(t)
		{
		}

		friend std::ios& operator<< (std::ios& io, const StreamManipulator<T>& m)
		{
			return m.m_manip(io, m.m_value);
		}

	private:
		Manipulator m_manip;
		T m_value;
	};

	template<class T>
	class IstreamManipulator
	{
	public:
		typedef std::istream&(*Manipulator)(std::istream&, T);

		explicit IstreamManipulator(Manipulator m, T t) :
			m_manip(m), m_value(t)
		{
		}

		friend std::istream& operator<< (std::istream& io, const IstreamManipulator<T>& m)
		{
			return m.m_manip(io, m.m_value);
		}

	private:
		Manipulator m_manip;
		T m_value;
	};

	template<class T>
	class OstreamManipulator
	{
	public:
		typedef std::ostream&(*Manipulator)(std::ostream&, T);

		explicit OstreamManipulator(Manipulator m, T t) :
			m_manip(m), m_value(t)
		{
		}

		friend std::ostream& operator<< (std::ostream& io, const OstreamManipulator<T>& m)
		{
			return m.m_manip(io, m.m_value);
		}

	private:
		Manipulator m_manip;
		T m_value;
	};


	template<class T>
	OstreamManipulator<T> hex(T t)
	{
		OstreamManipulator<T>::Manipulator m =
			[](std::ostream& o, T t) -> std::ostream&
			{
				const char map[] = "0123456789abcdef";
				const unsigned char* ptr = reinterpret_cast<const unsigned char*>(&t);
				for (size_t i = 0; i < sizeof(T); ++i)
				{
					unsigned first = ptr[i] / 16;
					unsigned second = ptr[i] - (first * 16);
					o << map[first] << map[second];
				}
				return o;
			};

		return OstreamManipulator<T>(m, t);
	}

	inline OstreamManipulator<const char*> format(const char* f)
	{
		OstreamManipulator<const char*>::Manipulator m =
			[](std::ostream& os, const char* fmt) -> std::ostream&
			{
				std::locale& loc = os.getloc();
				int i = 0;
				while (fmt[i] != 0)
				{
					if (fmt[i] != '%')
					{
						os << fmt[i];
						i++;
					}
					else
					{
						i++;
						if (fmt[i] == '%')
						{
							os << fmt[i];
							i++;
						}
						else
						{
							bool ok = true;
							int istart = i;
							bool more = true;
							int width = 0;
							int precision = 6;
							std::ios_base::fmtflags flags;
							char fill = ' ';
							bool alternate = false;
							while (more)
							{
								switch (fmt[i])
								{
								case '+':
									flags |= std::ios::showpos;
									break;
								case '-':
									flags |= std::ios::left;
									break;
								case '0':
									flags |= std::ios::internal;
									fill = '0';
									break;
								case '#':
									alternate = true;
									break;
								case ' ':
									break;
								default:
									more = false;
									break;
								}
								if (more) i++;
							}
							if (std::isdigit(fmt[i], loc))
							{
								width = std::atoi(fmt + i);
								do i++;
								while (std::isdigit(fmt[i], loc));
							}
							if (fmt[i] == '.')
							{
								i++;
								precision = std::atoi(fmt + i);
								while (std::isdigit(fmt[i], loc)) i++;
							}
							switch (fmt[i])
							{
							case 'd':
								flags |= std::ios::dec;
								break;
							case 'x':
								flags |= std::ios::hex;
								if (alternate) flags |= std::ios::showbase;
								break;
							case 'X':
								flags |= std::ios::hex | std::ios::uppercase;
								if (alternate) flags |= std::ios::showbase;
								break;
							case 'o':
								flags |= std::ios::hex;
								if (alternate) flags |= std::ios::showbase;
								break;
							case 'f':
								flags |= std::ios::fixed;
								if (alternate) flags |= std::ios::showpoint;
								break;
							case 'e':
								flags |= std::ios::scientific;
								if (alternate) flags |= std::ios::showpoint;
								break;
							case 'E':
								flags |= std::ios::scientific | std::ios::uppercase;
								if (alternate) flags |= std::ios::showpoint;
								break;
							case 'g':
								if (alternate) flags |= std::ios::showpoint;
								break;
							case 'G':
								flags |= std::ios::uppercase;
								if (alternate) flags |= std::ios::showpoint;
								break;
							default:
								ok = false;
								break;
							}
							i++;
							if (fmt[i] != 0) ok = false;
							if (ok)
							{
								os.unsetf(std::ios::adjustfield | std::ios::basefield |
									std::ios::floatfield);
								os.setf(flags);
								os.width(width);
								os.precision(precision);
								os.fill(fill);
							}
							else i = istart;
						}
					}
				}
				return os;
			};

		return OstreamManipulator<const char*>(m, f);
	}

	inline IstreamManipulator<char> delimiter(char d)
	{
		struct CustomLocale : std::ctype<char>
		{
			std::ctype_base::mask m_rc[table_size];

			CustomLocale(char delim) :
				std::ctype<char>(get_table(static_cast<unsigned char>(delim)))
			{
			}

			const std::ctype_base::mask* get_table(unsigned char delim)
			{
				memset(m_rc, 0, sizeof(std::ctype_base::mask) * table_size);
				m_rc[delim] = std::ctype_base::space;
				m_rc['\n'] = std::ctype_base::space;
				return &m_rc[0];
			}
		};

		IstreamManipulator<char>::Manipulator m =
			[](std::istream& i, char d) -> std::istream&
			{
				i.imbue(std::locale(i.getloc(), new CustomLocale(d)));
				return i;
			};

		return IstreamManipulator<char>(m, d);
	}

	inline IstreamManipulator<char> next(char d)
	{
		IstreamManipulator<char>::Manipulator m =
			[](std::istream& i, char d) -> std::istream&
			{
#pragma push_macro("__raz")
#undef max
				i.ignore(std::numeric_limits<std::streamsize>::max(), d);
#pragma pop_macro("__raz")
				return i;
			};

		return IstreamManipulator<char>(m, d);
	}

	inline std::istream& nextLine(std::istream& i)
	{
		return (i << next('\n'));
	}


	template<class Param>
	std::tuple<Param> parse(std::istream& i, char* d = nullptr)
	{
		Param p;
		if (d) i << delimiter(*d);
		if (!static_cast<bool>(i >> extract(p)))
			throw std::logic_error("parse error: " + typeid(Param).name());
		return std::tuple<Param> { p };
	}

	template<class Param1, class Param2, class... Params>
	std::tuple<Param1, Param2, Params...> parse(std::istream& i, char* d = nullptr)
	{
		if (d) i << delimiter(*d);
		auto a = parse<Param1>(i);
		auto b = parse<Param2, Params...>(i);
		return std::tuple_cat(a, b);
	}

	template<class... Params>
	std::tuple<Params...> parse(std::string str, char* d = nullptr)
	{
		std::stringstream ss(str);
		if (d) ss << delimiter(*d);
		return parse<Params...>(ss);
	}
};
