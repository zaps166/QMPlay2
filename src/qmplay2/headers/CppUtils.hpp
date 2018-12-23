/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2017  Błażej Szczygieł

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published
	by the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <type_traits>

template<typename T>
void asConst(const T &t) = delete;

template<typename T>
constexpr typename std::add_const<T>::type &asConst(T &t) noexcept
{
	return t;
}

template<typename T>
void asConst(const T &&t) = delete;

template<typename T>
void asConst(T &&t) = delete;


template <typename... Args>
struct Overload
{
	template <typename R, typename T>
	constexpr auto operator()(R (T::*ptr)(Args...)) const noexcept -> decltype(ptr)
	{
		return ptr;
	}

	template <typename R, typename T>
	static constexpr auto of(R (T::*ptr)(Args...)) noexcept -> decltype(ptr)
	{
		return ptr;
	}
};
