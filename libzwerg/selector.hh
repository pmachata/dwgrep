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

#include <type_traits>
#include <climits>
#include <vector>

#include "value.hh"

struct stack;

struct selector
{
  typedef uint32_t sel_t;
  static auto const W = sizeof (sel_t);

private:
  void unused(value_type &v)
  {
    static_assert (CHAR_BIT == 8, "character has 8 bits");
    static_assert (sizeof (v.code ()) == 1, "sizeof of value_type code is 1");
  }

  sel_t m_imprint;
  sel_t m_mask;

  static sel_t
  compute_mask ()
  {
    return 0;
  }

  template <class... Ts>
  static sel_t
  compute_mask (uint8_t code, Ts... codes)
  {
    return compute_mask (codes...) >> 8
      | (code != 0 ? ((sel_t) 0xff) << (8 * W - 8) : 0);
  }

  static sel_t
  compute_imprint ()
  {
    return 0;
  }

  template <class... Ts>
  static sel_t
  compute_imprint (uint8_t code, Ts... codes)
  {
    return compute_imprint (codes...) >> 8
      | (((sel_t) code) << (8 * W - 8));
  }

  static sel_t
  unshift (sel_t sel, size_t N)
  {
    return sel >> ((W - N) * 8);
  }

public:
  template <class... Ts,
	    typename std::enable_if <(sizeof... (Ts) <= W), int>::type Fake = 0>
  selector (Ts... vts)
    : m_imprint {unshift (compute_imprint ((vts.code ())...), sizeof... (Ts))}
    , m_mask {unshift (compute_mask ((vts.code ())...), sizeof... (Ts))}
  {}

  selector (stack const &s);

  selector (selector const &that)
    : m_imprint (that.m_imprint)
    , m_mask (that.m_mask)
  {}

  bool
  matches (selector const &profile) const
  {
    return (profile.m_imprint & m_mask) == m_imprint;
  }

  std::vector <value_type> get_types () const;

  bool operator< (selector const &that) const
  { return m_imprint < that.m_imprint; }
  bool operator== (selector const &that) const
  { return m_imprint == that.m_imprint; }
  bool operator!= (selector const &that) const
  { return m_imprint != that.m_imprint; }

  friend std::ostream &operator<< (std::ostream &o, selector const &sel);
};

std::ostream &operator<< (std::ostream &o, selector const &sel);

#endif /* _SELECTOR_H_ */
