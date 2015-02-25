/*
  Copyright (C) 2015 Red Hat, Inc.

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
#include "libzwerg-dw.h"
#include "libzwerg.hh"

#include "builtin-dw.hh"
#include "value-aset.hh"
#include "value-dw.hh"
#include "value-symbol.hh"
#include "dwcst.hh"

zw_machine *
zw_machine_init (int code)
{
  return reinterpret_cast <zw_machine *> (static_cast <uintptr_t> (code));
}

void
zw_machine_destroy (zw_machine *machine, zw_error **out_err)
{
  // NOP.  The API is written this way mostly for robustness and
  // homogeneity, but presently nothing is actually allocated.
}

int
zw_machine_code (zw_machine const *machine)
{
  return static_cast <int> (reinterpret_cast <uintptr_t> (machine));
}

zw_cdom const *
zw_cdom_dw_tag (void)
{
  static zw_cdom cdom {dw_tag_dom ()};
  return &cdom;
}

zw_cdom const *
zw_cdom_dw_attr (void)
{
  static zw_cdom cdom {dw_attr_dom ()};
  return &cdom;
}

zw_cdom const *
zw_cdom_dw_form (void)
{
  static zw_cdom cdom {dw_form_dom ()};
  return &cdom;
}

zw_cdom const *
zw_cdom_dw_lang (void)
{
  static zw_cdom cdom {dw_lang_dom ()};
  return &cdom;
}

zw_cdom const *
zw_cdom_dw_macinfo (void)
{
  static zw_cdom cdom {dw_macinfo_dom ()};
  return &cdom;
}

zw_cdom const *
zw_cdom_dw_macro (void)
{
  static zw_cdom cdom {dw_macro_dom ()};
  return &cdom;
}

zw_cdom const *
zw_cdom_dw_inline (void)
{
  static zw_cdom cdom {dw_inline_dom ()};
  return &cdom;
}

zw_cdom const *
zw_cdom_dw_encoding (void)
{
  static zw_cdom cdom {dw_encoding_dom ()};
  return &cdom;
}

zw_cdom const *
zw_cdom_dw_access (void)
{
  static zw_cdom cdom {dw_access_dom ()};
  return &cdom;
}

zw_cdom const *
zw_cdom_dw_visibility (void)
{
  static zw_cdom cdom {dw_visibility_dom ()};
  return &cdom;
}

zw_cdom const *
zw_cdom_dw_virtuality (void)
{
  static zw_cdom cdom {dw_virtuality_dom ()};
  return &cdom;
}

zw_cdom const *
zw_cdom_dw_identifier_case (void)
{
  static zw_cdom cdom {dw_identifier_case_dom ()};
  return &cdom;
}

zw_cdom const *
zw_cdom_dw_calling_convention (void)
{
  static zw_cdom cdom {dw_calling_convention_dom ()};
  return &cdom;
}

zw_cdom const *
zw_cdom_dw_ordering (void)
{
  static zw_cdom cdom {dw_ordering_dom ()};
  return &cdom;
}

zw_cdom const *
zw_cdom_dw_discr_list (void)
{
  static zw_cdom cdom {dw_discr_list_dom ()};
  return &cdom;
}

zw_cdom const *
zw_cdom_dw_decimal_sign (void)
{
  static zw_cdom cdom {dw_decimal_sign_dom ()};
  return &cdom;
}

zw_cdom const *
zw_cdom_dw_locexpr_opcode (void)
{
  static zw_cdom cdom {dw_locexpr_opcode_dom ()};
  return &cdom;
}

zw_cdom const *
zw_cdom_dw_address_class (void)
{
  static zw_cdom cdom {dw_address_class_dom ()};
  return &cdom;
}

zw_cdom const *
zw_cdom_dw_endianity (void)
{
  static zw_cdom cdom {dw_endianity_dom ()};
  return &cdom;
}

zw_cdom const *
zw_cdom_elfsym_stt (zw_machine const *machine)
{
  static zw_cdom cdom {elfsym_stt_dom (zw_machine_code (machine))};
  return &cdom;
}

zw_cdom const *
zw_cdom_elfsym_stb (zw_machine const *machine)
{
  static zw_cdom cdom {elfsym_stb_dom (zw_machine_code (machine))};
  return &cdom;
}

zw_cdom const *
zw_cdom_elfsym_stv (void)
{
  static zw_cdom cdom {elfsym_stv_dom ()};
  return &cdom;
}

zw_vocabulary const *
zw_vocabulary_dwarf (zw_error **out_err)
{
  return capture_errors ([&] () {
      static zw_vocabulary v {dwgrep_vocabulary_dw ()};
      return &v;
    }, nullptr, out_err);
}

bool
zw_value_is_dwarf (zw_value const *val)
{
  return val->is <value_dwarf> ();
}

bool
zw_value_is_cu (zw_value const *val)
{
  return val->is <value_cu> ();
}

bool
zw_value_is_die (zw_value const *val)
{
  return val->is <value_die> ();
}

bool
zw_value_is_attr (zw_value const *val)
{
  return val->is <value_attr> ();
}

bool
zw_value_is_llelem (zw_value const *val)
{
  return val->is <value_loclist_elem> ();
}

bool
zw_value_is_llop (zw_value const *val)
{
  return val->is <value_loclist_op> ();
}

bool
zw_value_is_aset (zw_value const *val)
{
  return val->is <value_aset> ();
}

bool
zw_value_is_elfsym (zw_value const *val)
{
  return val->is <value_symbol> ();
}

namespace
{
  zw_value *
  init_dwarf (char const *filename, doneness d, size_t pos, zw_error **out_err)
  {
    return capture_errors ([&] () {
	return std::make_unique <value_dwarf> (filename, pos, d).release ();
      }, nullptr, out_err);
  }
}

zw_value *
zw_value_init_dwarf (char const *filename, size_t pos, zw_error **out_err)
{
  return init_dwarf (filename, doneness::cooked, pos, out_err);
}

zw_value *
zw_value_init_dwarf_raw (char const *filename, size_t pos, zw_error **out_err)
{
  return init_dwarf (filename, doneness::raw, pos, out_err);
}

namespace
{
  value_dwarf const &
  dwarf (zw_value const *val)
  {
    return value::require_as <value_dwarf> (val);
  }
}

Dwfl *
zw_value_dwarf_dwfl (zw_value const *val)
{
  return dwarf (val).get_dwctx ()->get_dwfl ();
}

char const *
zw_value_dwarf_name (zw_value const *val)
{
  return dwarf (val).get_fn ().c_str ();
}

zw_machine const *
zw_value_dwarf_machine (zw_value const *val, zw_error **out_err)
{
  return capture_errors ([&] () {
      // This relies on the fact that zw_machine_init doesn't do any
      // real allocation, and thus doesn't store the generated machine
      // destriptor anywhere.
      return zw_machine_init (dwarf (val).get_dwctx ()->get_machine ());
    }, nullptr, out_err);
}


namespace
{
  value_cu const &
  cu (zw_value const *val)
  {
    return value::require_as <value_cu> (val);
  }
}

Dwarf_CU *
zw_value_cu_cu (zw_value const *val)
{
  return &cu (val).get_cu ();
}

Dwarf_Off
zw_value_cu_offset (zw_value const *val)
{
  return cu (val).get_offset ();
}

namespace
{
  value_die const &
  die (zw_value const *val)
  {
    return value::require_as <value_die> (val);
  }
}

Dwarf_Die
zw_value_die_die (zw_value const *val)
{
  return die (val).get_die ();
}

zw_value const *
zw_value_die_dwarf (zw_value const *val, zw_error **out_err)
{
  return capture_errors ([&] () {
      // get_dwarf caches things, so we need to cast away the const.
      return &const_cast <value_die &> (die (val)).get_dwarf ();
    }, nullptr, out_err);
}

namespace
{
  value_attr const &
  attr (zw_value const *val)
  {
    return value::require_as <value_attr> (val);
  }
}

Dwarf_Attribute
zw_value_attr_attr (zw_value const *val)
{
  return attr (val).get_attr ();
}

zw_value const *
zw_value_attr_dwarf (zw_value const *val, zw_error **out_err)
{
  return capture_errors ([&] () {
      // get_dwarf caches things, so we need to cast away the const.
      return &const_cast <value_attr &> (attr (val)).get_dwarf ();
    }, nullptr, out_err);
}


namespace
{
  value_loclist_elem const &
  llelem (zw_value const *val)
  {
    return value::require_as <value_loclist_elem> (val);
  }
}

Dwarf_Attribute
zw_value_llelem_attribute (zw_value const *val)
{
  return llelem (val).get_attr ();
}

Dwarf_Addr
zw_value_llelem_low (zw_value const *val)
{
  return llelem (val).get_low ();
}

Dwarf_Addr
zw_value_llelem_high (zw_value const *val)
{
  return llelem (val).get_high ();
}

Dwarf_Op *
zw_value_llelem_expr (zw_value const *val, size_t *out_length)
{
  auto const &e = llelem (val);
  *out_length = e.get_exprlen ();
  return e.get_expr ();
}


namespace
{
  value_loclist_op const &
  llop (zw_value const *val)
  {
    return value::require_as <value_loclist_op> (val);
  }
}

Dwarf_Attribute
zw_value_llop_attribute (zw_value const *val)
{
  return llop (val).get_attr ();
}

Dwarf_Op *
zw_value_llop_op (zw_value const *val)
{
  return llop (val).get_dwop ();
}


namespace
{
  value_aset const &
  aset (zw_value const *val)
  {
    return value::require_as <value_aset> (val);
  }
}

size_t
zw_value_aset_length (zw_value const *val)
{
  return aset (val).get_coverage ().size ();
}

struct zw_aset_pair
zw_value_aset_at (zw_value const *val, size_t idx)
{
  auto const &cov = aset (val).get_coverage ();
  assert (idx < cov.size ());
  auto const &range = cov.at (idx);
  return {range.start, range.length};
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
