/*
   Copyright (C) 2009, 2010, 2011, 2012, 2014 Red Hat, Inc.
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

#ifndef DWIT_H
#define DWIT_H

#include <vector>
#include <cassert>
#include <algorithm>
#include <dwarf.h>

#include "dwpp.hh"

class dwfl_module_iterator
  : public std::iterator<std::input_iterator_tag, Dwfl_Module *>
{
  Dwfl *m_dwfl;
  ptrdiff_t m_offset;
  Dwarf *m_ret_dw;
  Dwarf_Addr m_ret_bias;

  static int
  module_cb (Dwfl_Module *mod, void **data, const char *name,
	     Dwarf_Addr addr, void *arg)
  {
    auto self = static_cast <dwfl_module_iterator *> (arg);
    self->m_ret_dw = dwfl_module_getdwarf (mod, &self->m_ret_bias);
    if (self->m_ret_dw == nullptr)
      throw_libdwfl ();
    return DWARF_CB_ABORT;
  }

  void
  move ()
  {
    m_offset = dwfl_getmodules (m_dwfl, module_cb, this, m_offset);
    if (m_offset == -1)
      throw_libdwfl ();
  }

  explicit dwfl_module_iterator (ptrdiff_t off)
    : m_dwfl {nullptr}
    , m_offset {off}
  {}

public:
  dwfl_module_iterator (Dwfl *dwfl)
    : m_dwfl {dwfl}
    , m_offset {0}
  {
    move ();
  }

  dwfl_module_iterator (dwfl_module_iterator const &that) = default;

  static dwfl_module_iterator
  end ()
  {
    return dwfl_module_iterator ((ptrdiff_t) 0);
  }

  dwfl_module_iterator &
  operator++ ()
  {
    assert (*this != end ());
    move ();
    return *this;
  }

  dwfl_module_iterator
  operator++ (int)
  {
    auto ret = *this;
    ++*this;
    return ret;
  }

  std::pair <Dwarf *, Dwarf_Addr>
  operator* () const
  {
    return std::make_pair (m_ret_dw, m_ret_bias);
  }

  bool
  operator== (dwfl_module_iterator const &that) const
  {
    assert (m_dwfl == nullptr || that.m_dwfl == nullptr
	    || m_dwfl == that.m_dwfl);
    return m_offset == that.m_offset;
  }

  bool
  operator!= (dwfl_module_iterator const &that) const
  {
    return !(*this == that);
  }
};

class cu_iterator
  : public std::iterator<std::input_iterator_tag, Dwarf_Die *>
{
  friend class all_dies_iterator;
  Dwarf *m_dw;
  Dwarf_Off m_offset;
  Dwarf_Die m_cudie;

  explicit cu_iterator (Dwarf_Off off)
    : m_dw (nullptr)
    , m_offset (off)
    , m_cudie ({})
  {}

  void
  move ()
  {
    assert (*this != end ());
    do
      {
	Dwarf_Off old_offset = m_offset;
	size_t hsize;
	if (dwarf_nextcu (m_dw, m_offset, &m_offset, &hsize,
			  nullptr, nullptr, nullptr) != 0)
	  done ();
	else if (dwarf_offdie (m_dw, old_offset + hsize, &m_cudie) == nullptr)
	  continue;
	else
	  // XXX partial unit, type unit, what else?
	  assert (dwarf_tag (&m_cudie) == DW_TAG_compile_unit);
      }
    while (false);
  }

  void
  done ()
  {
    *this = end ();
  }

public:
  cu_iterator (cu_iterator const &other) = default;

  explicit cu_iterator (Dwarf *dw)
    : m_dw {dw}
    , m_offset {0}
    , m_cudie {}
  {
    move ();
  }

  cu_iterator (Dwarf *dw, Dwarf_Die cudie)
    : m_dw {dw}
    , m_offset {dwarf_dieoffset (&cudie) - dwarf_cuoffset (&cudie)}
    , m_cudie {}
  {
    move ();
  }

  static cu_iterator
  end ()
  {
    return cu_iterator ((Dwarf_Off)-1);
  }

  bool
  operator== (cu_iterator const &other) const
  {
    return m_offset == other.m_offset;
  }

  bool
  operator!= (cu_iterator const &other) const
  {
    return !(*this == other);
  }

  cu_iterator
  operator++ ()
  {
    move ();
    return *this;
  }

  cu_iterator
  operator++ (int)
  {
    cu_iterator tmp = *this;
    ++*this;
    return tmp;
  }

  Dwarf_Off
  offset () const
  {
    return m_offset;
  }

  Dwarf_Die *
  operator* ()
  {
    return &m_cudie;
  }
};

// Tree flattening iterator.  It pre-order iterates all DIEs in given
// dwarf file.
class all_dies_iterator
  : public std::iterator<std::input_iterator_tag, Dwarf_Die *>
{
  cu_iterator m_cuit;
  std::vector<Dwarf_Off> m_stack;
  Dwarf_Die m_die;

  all_dies_iterator (Dwarf_Off offset)
    : m_cuit (cu_iterator::end ())
  {
  }

public:

  all_dies_iterator (all_dies_iterator const &other) = default;

  explicit all_dies_iterator (Dwarf *dw)
    : all_dies_iterator (cu_iterator {dw})
  {}

  explicit all_dies_iterator (cu_iterator const &cuit)
    : m_cuit (cuit)
    , m_die (**m_cuit)
  {}

  static all_dies_iterator
  end ()
  {
    return all_dies_iterator ((Dwarf_Off)-1);
  }

  bool
  operator== (all_dies_iterator const &other) const
  {
    return m_cuit == other.m_cuit
      && m_stack == other.m_stack
      && (m_cuit == cu_iterator::end ()
	  || m_die.addr == other.m_die.addr);
  }

  bool
  operator!= (all_dies_iterator const &other) const
  {
    return !(*this == other);
  }

  all_dies_iterator
  operator++ ()
  {
    if (dwarf_haschildren (&m_die))
      {
	m_stack.push_back (dwarf_dieoffset (&m_die));
	if (dwarf_child (&m_die, &m_die))
	  throw_libdw ();
	return *this;
      }

    do
      switch (dwarf_siblingof (&m_die, &m_die))
	{
	case -1:
	  throw_libdw ();
	case 0:
	  return *this;
	case 1:
	  // No sibling found.  Go a level up and retry, unless this
	  // was a sole, childless CU DIE.
	  if (! m_stack.empty ())
	    {
	      if (dwarf_offdie (m_cuit.m_dw, m_stack.back (), &m_die) == nullptr)
		throw_libdw ();
	      m_stack.pop_back ();
	    }
	}
    while (!m_stack.empty ());

    m_die = **++m_cuit;
    return *this;
  }

  all_dies_iterator
  operator++ (int)
  {
    all_dies_iterator prev = *this;
    ++*this;
    return prev;
  }

  Dwarf_Die *
  operator* ()
  {
    return &m_die;
  }

  std::vector<Dwarf_Die>
  stack () const
  {
    std::vector<Dwarf_Die> ret;
    for (all_dies_iterator it = *this; it != end (); it = it.parent ())
      ret.push_back (**it);
    std::reverse (ret.begin (), ret.end ());
    return ret;
  }

  all_dies_iterator
  parent () const
  {
    assert (*this != end ());
    if (m_stack.empty ())
      return end ();

    all_dies_iterator ret = *this;
    if (dwarf_offdie (m_cuit.m_dw, m_stack.back (), &ret.m_die) == nullptr)
      throw_libdw ();
    ret.m_stack.pop_back ();
    return ret;
  }

  cu_iterator
  cu () const
  {
    return m_cuit;
  }
};

class attr_iterator
  : public std::iterator<std::input_iterator_tag, Dwarf_Attribute *>
{
  Dwarf_Die *m_die;
  Dwarf_Attribute m_at;
  ptrdiff_t m_offset;

  struct cb_data
  {
    Dwarf_Attribute *at;
    bool been;
  };

  static int
  callback (Dwarf_Attribute *at, void *data)
  {
    cb_data *d = static_cast<cb_data *> (data);
    if (d->been)
      return DWARF_CB_ABORT;

    *d->at = *at;
    d->been = true;

    // Do a second iteration to find the next offset.
    return DWARF_CB_OK;
  }

  void
  move ()
  {
    // If m_offset is already 1, we are done iterating.
    if (m_offset == 1)
      {
	*this = end ();
	return;
      }

    cb_data data = {&m_at, false};
    m_offset = dwarf_getattrs (m_die, &callback, &data, m_offset);
    if (m_offset == -1)
      throw_libdw ();
  }

  attr_iterator (ptrdiff_t offset)
    : m_die (nullptr)
    , m_at ({0})
    , m_offset (offset)
  {}

public:
  attr_iterator (Dwarf_Die *die)
    : m_die (die)
    , m_at ({0})
    , m_offset (0)
  {
    move ();
  }

  bool
  operator== (attr_iterator const &other) const
  {
    return m_offset == other.m_offset
      && m_at.code == other.m_at.code;
  }

  bool
  operator!= (attr_iterator const &other) const
  {
    return !(*this == other);
  }

  attr_iterator &
  operator++ ()
  {
    assert (*this != end ());
    move ();
    return *this;
  }

  attr_iterator
  operator++ (int)
  {
    attr_iterator tmp = *this;
    ++*this;
    return tmp;
  }

  Dwarf_Attribute *
  operator* ()
  {
    return &m_at;
  }

  static attr_iterator
  end ()
  {
    return attr_iterator ((ptrdiff_t)1);
  }
};

#endif /* DWIT_H */
