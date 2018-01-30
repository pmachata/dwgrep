/*
   Copyright (C) 2018 Petr Machata
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

#ifndef VALUE_ELF_H
#define VALUE_ELF_H

#include "value.hh"
#include "dwfl_context.hh"
#include "value-dw.hh"

class value_elf
  : public value
  , public doneness_aspect
{
  std::string m_fn;
  std::shared_ptr <dwfl_context> m_dwctx;
  dwarf_value_cache m_dwcache;

public:
  static value_type const vtype;

  value_elf (std::shared_ptr <dwfl_context> dwctx, std::string const &fn,
	     size_t pos, doneness d)
    : value {vtype, pos}
    , doneness_aspect {d}
    , m_fn {fn}
    , m_dwctx {dwctx}
  {}

  std::string &get_fn ()
  { return m_fn; }

  std::string const &get_fn () const
  { return m_fn; }

  std::shared_ptr <dwfl_context> get_dwctx () const
  { return m_dwctx; }

  void show (std::ostream &o) const override;
  cmp_result cmp (value const &that) const override;
  std::unique_ptr <value> clone () const override;

  value_dwarf &
  get_dwarf ()
  {
    return m_dwcache.get_dwarf (m_dwctx, 0, get_doneness (), get_fn ());
  }
};

#endif /* VALUE_ELF_H */
