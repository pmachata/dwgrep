/*
   Copyright (C) 2014 Red Hat, Inc.
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

#ifndef _FLAG_SAVER_H_
#define _FLAG_SAVER_H_

#include <iostream>

class ios_flag_saver
{
  std::ios &m_stream;
  std::ios::fmtflags m_flags;

public:
  explicit ios_flag_saver (std::ios &stream)
    : m_stream {stream}
    , m_flags {stream.flags ()}
  {}

  ~ios_flag_saver ()
  {
    m_stream.flags (m_flags);
  }
};

#endif /* _FLAG_SAVER_H_ */
