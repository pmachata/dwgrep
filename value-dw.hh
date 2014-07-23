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

#ifndef _VALUE_DW_H_
#define _VALUE_DW_H_

#include <elfutils/libdw.h>
#include "value.hh"

class value_die
  : public value
{
  Dwarf_Die m_die;
  dwgrep_graph::sptr m_gr;

public:
  static value_type const vtype;

  value_die (dwgrep_graph::sptr gr, Dwarf_Die die, size_t pos)
    : value {vtype, pos}
    , m_die (die)
    , m_gr {gr}
  {}

  value_die (value_die const &that) = default;

  Dwarf_Die &get_die ()
  { return m_die; }

  void show (std::ostream &o, bool full) const override;
  std::unique_ptr <value> clone () const override;
  cmp_result cmp (value const &that) const override;
};

class value_attr
  : public value
{
  Dwarf_Attribute m_attr;
  Dwarf_Die m_die;
  dwgrep_graph::sptr m_gr;

public:
  static value_type const vtype;

  value_attr (dwgrep_graph::sptr gr,
	      Dwarf_Attribute attr, Dwarf_Die die, size_t pos)
    : value {vtype, pos}
    , m_attr (attr)
    , m_die (die)
    , m_gr {gr}
  {}

  value_attr (value_attr const &that) = default;

  Dwarf_Attribute &get_attr ()
  { return m_attr; }

  Dwarf_Die &get_die ()
  { return m_die; }

  void show (std::ostream &o, bool full) const override;
  std::unique_ptr <value> clone () const override;
  cmp_result cmp (value const &that) const override;
};

class value_loclist_op
  : public value
{
  // This apparently wild pointer points into libdw-private data.  We
  // actually need to carry a pointer, as some functions require that
  // they be called with the original pointer, not our own copy.
  Dwarf_Op *m_dwop;
  Dwarf_Attribute m_attr;
  dwgrep_graph::sptr m_gr;

public:
  static value_type const vtype;

  value_loclist_op (dwgrep_graph::sptr gr,
		    Dwarf_Op *dwop, Dwarf_Attribute attr, size_t pos)
    : value {vtype, pos}
    , m_dwop (dwop)
    , m_attr (attr)
    , m_gr {gr}
  {}

  value_loclist_op (value_loclist_op const &that) = default;

  Dwarf_Attribute &get_attr ()
  { return m_attr; }

  Dwarf_Op *get_dwop ()
  { return m_dwop; }

  void show (std::ostream &o, bool full) const override;
  std::unique_ptr <value> clone () const override;
  cmp_result cmp (value const &that) const override;
};

#endif /* _VALUE_DW_H_ */
