/*
   Copyright (C) 2018 Petr Machata
   Copyright (C) 2014, 2015 Red Hat, Inc.
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

#ifndef VALUE_SYMBOL_H
#define VALUE_SYMBOL_H

#include <gelf.h>

#include "value.hh"
#include "dwfl_context.hh"
#include "value-dw.hh"

class value_symbol
  : public value
  , public doneness_aspect
{
  std::shared_ptr <dwfl_context> m_dwctx;
  GElf_Sym m_symbol;
  char const *m_name;
  dwarf_value_cache m_dwcache;
  unsigned m_symidx;

public:
  static value_type const vtype;

  value_symbol (std::shared_ptr <dwfl_context> dwctx, GElf_Sym symbol,
		char const *name, unsigned symidx, size_t pos, doneness d)
    : value {vtype, pos}
    , doneness_aspect {d}
    , m_dwctx {dwctx}
    , m_symbol (symbol)
    , m_name {name}
    , m_symidx {symidx}
  {}

  std::shared_ptr <dwfl_context> get_dwctx () const
  { return m_dwctx; }

  char const *get_name () const
  { return m_name; }

  GElf_Sym get_symbol () const
  { return m_symbol; }

  unsigned get_symidx () const
  { return m_symidx; }

  void show (std::ostream &o) const override;
  std::unique_ptr <value> clone () const override;
  cmp_result cmp (value const &that) const override;

  value_dwarf &
  get_dwarf ()
  {
    return m_dwcache.get_dwarf (m_dwctx, 0, get_doneness ());
  }

  constant get_type () const;
  constant get_address () const;
};

constant_dom const &elfsym_stt_dom (int machine);
constant_dom const &elfsym_stb_dom (int machine);
constant_dom const &elfsym_stv_dom ();

#endif /* VALUE_SYMBOL_H */
