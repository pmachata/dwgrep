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

  static int module_cb (Dwfl_Module *mod, void **data, const char *name,
			Dwarf_Addr addr, void *arg);

  void move ();

  explicit dwfl_module_iterator (ptrdiff_t off);

public:
  dwfl_module_iterator (Dwfl *dwfl);
  dwfl_module_iterator (dwfl_module_iterator const &that) = default;

  static dwfl_module_iterator end ();

  dwfl_module_iterator &operator++ ();
  dwfl_module_iterator operator++ (int);

  std::pair <Dwarf *, Dwarf_Addr> operator* () const;

  bool operator== (dwfl_module_iterator const &that) const;
  bool operator!= (dwfl_module_iterator const &that) const;
};

class cu_iterator
  : public std::iterator<std::input_iterator_tag, Dwarf_Die *>
{
  friend class all_dies_iterator;
  Dwarf *m_dw;
  Dwarf_Off m_offset;
  Dwarf_Off m_old_offset;
  Dwarf_Die m_cudie;

  explicit cu_iterator (Dwarf_Off off);

  void move ();
  void done ();

public:
  explicit cu_iterator (Dwarf *dw);
  cu_iterator (Dwarf *dw, Dwarf_Die cudie);
  cu_iterator (cu_iterator const &other) = default;

  static cu_iterator end ();

  bool operator== (cu_iterator const &other) const;
  bool operator!= (cu_iterator const &other) const;

  cu_iterator operator++ ();
  cu_iterator operator++ (int);

  Dwarf_Die *operator* ();

  Dwarf_Off offset () const;
};

class child_iterator
  : public std::iterator <std::input_iterator_tag, Dwarf_Die *>
{
  Dwarf_Die m_die;

  child_iterator ();

public:
  explicit child_iterator (Dwarf_Die parent);
  child_iterator (child_iterator const &other) = default;

  static child_iterator end ();

  bool operator== (child_iterator const &other) const;
  bool operator!= (child_iterator const &other) const;

  child_iterator operator++ ();
  child_iterator operator++ (int);

  Dwarf_Die *operator* ();
};

// Tree flattening iterator.  It pre-order iterates all DIEs in given
// dwarf file.
class all_dies_iterator
  : public std::iterator<std::input_iterator_tag, Dwarf_Die *>
{
  cu_iterator m_cuit;
  std::vector<Dwarf_Off> m_stack;
  Dwarf_Die m_die;

  all_dies_iterator (Dwarf_Off offset);

public:
  explicit all_dies_iterator (Dwarf *dw);
  explicit all_dies_iterator (cu_iterator const &cuit);
  all_dies_iterator (all_dies_iterator const &other) = default;

  static all_dies_iterator end ();

  bool operator== (all_dies_iterator const &other) const;
  bool operator!= (all_dies_iterator const &other) const;

  all_dies_iterator operator++ ();
  all_dies_iterator operator++ (int);

  Dwarf_Die *operator* ();

  std::vector<Dwarf_Die> stack () const;
  all_dies_iterator parent () const;
  cu_iterator cu () const;
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
