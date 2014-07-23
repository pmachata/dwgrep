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

#include "valfile.hh"

void
frame::bind_value (var_id index, std::unique_ptr <value> val)
{
  assert (index < m_values.size ());
  // XXX this might actually be an assertion.  Cases of this should be
  // statically determinable.
  if (m_values[index] != nullptr)
    throw std::runtime_error ("attempt to rebind a bound variable");

  m_values[index] = std::move (val);
}

value &
frame::read_value (var_id index)
{
  assert (index < m_values.size ());
  // XXX this might actually be an assertion.  Cases of this should be
  // statically determinable.
  if (m_values[index] == nullptr)
    throw std::runtime_error ("attempt to read an unbound variable");

  return *m_values[index];
}

std::shared_ptr <frame>
frame::clone () const
{
  auto ret = std::make_shared <frame> (m_parent, 0);
  for (auto const &val: m_values)
    ret->m_values.push_back (val != nullptr ? val->clone () : nullptr);
  return ret;
}

valfile::valfile (valfile const &that)
  : m_frame {that.m_frame != nullptr ? that.m_frame->clone () : nullptr}
{
  for (auto const &vf: that.m_values)
    m_values.push_back (vf->clone ());
}

namespace
{
  int
  compare_vf (std::vector <std::unique_ptr <value> > const &a,
	      std::vector <std::unique_ptr <value> > const &b)
  {
    if (a.size () < b.size ())
      return -1;
    else if (a.size () > b.size ())
      return 1;

    // The stack that has nullptr where the other has non-nullptr is
    // smaller.
    {
      auto it = a.begin ();
      auto jt = b.begin ();
      for (; it != a.end (); ++it, ++jt)
	if (*it == nullptr && *jt != nullptr)
	  return -1;
	else if (*it != nullptr && *jt == nullptr)
	  return 1;
    }

    // The stack with "smaller" types is smaller.
    {
      auto it = a.begin ();
      auto jt = b.begin ();
      for (; it != a.end (); ++it, ++jt)
	if (*it != nullptr && *jt != nullptr)
	  {
	    if ((*it)->get_type () < (*jt)->get_type ())
	      return -1;
	    else if ((*jt)->get_type () < (*it)->get_type ())
	      return 1;
	  }
    }

    // We have the same number of slots with values of the same type.
    // Now compare the values directly.
    {
      auto it = a.begin ();
      auto jt = b.begin ();
      for (; it != a.end (); ++it, ++jt)
	if (*it != nullptr && *jt != nullptr)
	  switch ((*it)->cmp (**jt))
	    {
	    case cmp_result::fail:
	      assert (! "Comparison of same-typed slots shouldn't fail!");
	      abort ();
	    case cmp_result::less:
	      return -1;
	    case cmp_result::greater:
	      return 1;
	    case cmp_result::equal:
	      break;
	    }
    }

    // The stacks are the same!
    return 0;
  }
}

bool
valfile::operator< (valfile const &that) const
{
  return compare_vf (m_values, that.m_values) < 0;
}

bool
valfile::operator== (valfile const &that) const
{
  return compare_vf (m_values, that.m_values) == 0;
}
