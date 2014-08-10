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

#ifndef _SELECTOR_H_
#define _SELECTOR_H_

#include <array>
#include <type_traits>

#include "value.hh"

struct stack;
class selector
{
  union
  {
    std::array <value_type, 4> m_vts;
    uint32_t m_imprint;
  };
  union
  {
    std::array <uint8_t, 4> m_mask_elts;
    uint32_t m_mask;
  };

  static_assert (sizeof (m_vts) == 4, "assuming 4 value_types in uint32_t");

  static uint32_t
  compute_mask (uint8_t code)
  {
    return code != 0 ? 0xff : 0;
  }

  template <class... Ts>
  static uint32_t
  compute_mask (uint8_t code, Ts... codes)
  {
    return compute_mask (codes...) << 8 | compute_mask (code);
  }

  static std::array <value_type, 4> get_vts (stack const &s);

public:
  template <class... Ts, std::enable_if_t <(sizeof... (Ts) == 4), int> Fake = 0>
  selector (Ts... vts)
    : m_vts {{vts...}}
    , m_mask_elts {{(vts.code () != 0 ? (uint8_t) 0xff : (uint8_t) 0)...}}
  {}

  template <class... Ts, std::enable_if_t <(sizeof... (Ts) < 4), int> Fake = 0>
  selector (Ts... vts)
    : selector {value_type {0}, vts...}
  {}

  selector (stack const &s);

  selector (selector const &that)
    : m_vts (that.m_vts)
    , m_mask_elts (that.m_mask_elts)
  {}

  bool
  matches (selector const &profile) const
  {
    return (profile.m_imprint & m_mask) == m_imprint;
  }

  bool operator< (selector const &that) const { return m_vts < that.m_vts; }
  bool operator== (selector const &that) const { return m_vts == that.m_vts; }
  bool operator!= (selector const &that) const { return m_vts != that.m_vts; }

  friend std::ostream &operator<< (std::ostream &o, selector const &sel);
};

std::ostream &operator<< (std::ostream &o, selector const &sel);

#endif /* _SELECTOR_H_ */
