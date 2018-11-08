/*
  Copyright (C) 2018 Petr Machata

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

#include "libzwergP.hh"
#include "libzwerg-elf.h"
#include "libzwerg.hh"

#include "builtin-elf-voc.hh"
#include "value-elf.hh"
#include "value-symbol.hh"

zw_cdom const *
zw_cdom_elfsym_stt (zw_machine const *machine)
{
  return &elfsym_stt_dom (zw_machine_code (machine));
}

zw_cdom const *
zw_cdom_elfsym_stb (zw_machine const *machine)
{
  return &elfsym_stb_dom (zw_machine_code (machine));
}

zw_cdom const *
zw_cdom_elfsym_stv (void)
{
  return &elfsym_stv_dom ();
}

zw_vocabulary const *
zw_vocabulary_elf (zw_error **out_err)
{
  return capture_errors ([&] () {
      static zw_vocabulary v {dwgrep_vocabulary_elf ()};
      return &v;
    }, nullptr, out_err);
}

bool
zw_value_is_elfsym (zw_value const *val)
{
  return val->is <value_symbol> ();
}

bool
zw_value_is_elf (zw_value const *val)
{
  return val->is <value_elf> ();
}

bool
zw_value_is_elfscn (zw_value const *val)
{
  return val->is <value_elf_section> ();
}

namespace
{
  value_symbol const &
  elfsym (zw_value const *val)
  {
    return value::require_as <value_symbol> (val);
  }
}

unsigned
zw_value_elfsym_symidx (zw_value const *val)
{
  return elfsym (val).get_symidx ();
}

GElf_Sym
zw_value_elfsym_symbol (zw_value const *val)
{
  return elfsym (val).get_symbol ();
}

char const *
zw_value_elfsym_name (zw_value const *val)
{
  return elfsym (val).get_name ();
}

zw_value const *
zw_value_elfsym_dwarf (zw_value const *val, zw_error **out_err)
{
  return capture_errors ([&] () {
      // get_dwarf caches things, so we need to cast away the const.
      return &const_cast <value_symbol &> (elfsym (val)).get_dwarf ();
    }, nullptr, out_err);
}

namespace
{
  value_elf const &
  elfval (zw_value const *val)
  {
    return value::require_as <value_elf> (val);
  }
}

zw_value const *
zw_value_elf_dwarf (zw_value const *val, zw_error **out_err)
{
  return capture_errors ([&] () {
      // get_dwarf caches things, so we need to cast away the const.
      return &const_cast <value_elf &> (elfval (val)).get_dwarf ();
    }, nullptr, out_err);
}
