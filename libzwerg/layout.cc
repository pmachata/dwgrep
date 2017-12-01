/*
   Copyright (C) 2017 Petr Machata
   This file is part of dwgrep.

   This file is free software; you can redistribute it and/or modify
   it under the terms of either

     * the GNU Lesser General Public License as published by the Free
       Software Foundation; either version 3 of the License, or (at
       your option) any later version

   or

     * the GNU General Public License as published by the Free
       Software Foundation; either version 2 of the License, or (at
       your option) any later version

   or both in parallel, as here.

   dwgrep is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received copies of the GNU General Public License and
   the GNU Lesser General Public License along with this program.  If
   not, see <http://www.gnu.org/licenses/>.  */

#include "layout.hh"

namespace
{
  size_t
  align (size_t top, size_t align)
  {
    return (top + (align - 1)) & -align;
  }
}

layout::loc
layout::reserve (size_t size, size_t align)
{
  m_size = ::align (m_size, align);
  auto ret = loc {m_size};
  m_size += size;
  return ret;
}

void
layout::add_union (std::vector <layout> layouts)
{
  for (auto const &layout: layouts)
    if (layout.m_size > m_size)
      m_size = layout.m_size;
}

size_t
layout::size () const
{
  return m_size;
}
