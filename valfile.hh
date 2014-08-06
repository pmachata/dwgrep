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

#ifndef _VALFILE_H_
#define _VALFILE_H_

#include <memory>
#include <vector>

#include "value.hh"

enum var_id: unsigned {};

// Stack frame, or activation record, of a running procedure (or other
// sort of context).
struct frame
{
  std::shared_ptr <frame> m_parent;
  std::vector <std::unique_ptr <value>> m_values;

  frame (std::shared_ptr <frame> parent, size_t vars)
    : m_parent {parent}
    , m_values {vars}
  {}

  void bind_value (var_id index, std::unique_ptr <value> val);
  value &read_value (var_id index);

  std::shared_ptr <frame> clone () const;
};

// Value file is a container type that's used for maintaining stacks
// of dwgrep values.
class valfile
{
  std::vector <std::unique_ptr <value>> m_values;
  std::shared_ptr <frame> m_frame;

public:
  typedef std::unique_ptr <valfile> uptr;

  valfile () = default;
  valfile (valfile const &other);
  valfile (valfile &&other) = default;
  ~valfile () = default;

  std::shared_ptr <frame>
  nth_frame (size_t depth) const
  {
    auto ret = m_frame;
    for (size_t i = 0; i < depth; ++i)
      ret = ret->m_parent;
    return ret;
  }

  void
  set_frame (std::shared_ptr <frame> frame)
  {
    m_frame = frame;
  }

  size_t
  size () const
  {
    return m_values.size ();
  }

  void
  push (std::unique_ptr <value> vp)
  {
    m_values.push_back (std::move (vp));
  }

  std::unique_ptr <value>
  pop ()
  {
    assert (! m_values.empty ());
    auto ret = std::move (m_values.back ());
    m_values.pop_back ();
    return ret;
  }

  template <class T>
  std::unique_ptr <T>
  pop_as ()
  {
    auto vp = pop ();
    assert (vp->is <T> ());
    return std::unique_ptr <T> (static_cast <T *> (vp.release ()));
  }

  value &
  top ()
  {
    assert (! m_values.empty ());
    return *m_values.back ().get ();
  }

  value &
  get (unsigned depth)
  {
    assert (m_values.size () > depth);
    return *(m_values.rbegin () + depth)->get ();
  }

  value const &
  get (unsigned depth) const
  {
    assert (m_values.size () > depth);
    return *(m_values.rbegin () + depth)->get ();
  }

  template <class T>
  T *
  top_as ()
  {
    value const &ret = top ();
    return value::as <T> (const_cast <value *> (&ret));
  }

  template <class T>
  T *
  get_as (unsigned depth)
  {
    value const &ret = get (depth);
    return value::as <T> (const_cast <value *> (&ret));
  }

  bool operator< (valfile const &that) const;
  bool operator== (valfile const &that) const;
};

#endif /* _VALFILE_H_ */
