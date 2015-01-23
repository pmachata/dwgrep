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
#include "value-dw.hh"
#include "dwcst.hh"

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
  return libzw_value_is <value_dwarf> (val);
}

bool
zw_value_is_cu (zw_value const *val)
{
  return libzw_value_is <value_cu> (val);
}

bool
zw_value_is_die (zw_value const *val)
{
  return libzw_value_is <value_die> (val);
}

bool
zw_value_is_attr (zw_value const *val)
{
  return libzw_value_is <value_attr> (val);
}

namespace
{
  zw_value *
  init_dwarf (char const *filename, doneness d, size_t pos, zw_error **out_err)
  {
    return capture_errors ([&] () {
	return new zw_value {std::make_unique <value_dwarf> (filename, pos, d)};
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

char const *
zw_value_dwarf_name (zw_value const *val)
{
  assert (val != nullptr);

  value_dwarf const *dw = value::as <value_dwarf> (val->m_value.get ());
  assert (dw != nullptr);

  return dw->get_fn ().c_str ();
}

namespace
{
  template <class T>
  T const &
  unpack (zw_value const *val)
  {
    assert (val != nullptr);

    T const *hlv = value::as <T> (val->m_value.get ());
    assert (hlv != nullptr);

    return *hlv;
  }

  value_cu const &
  cu (zw_value const *val)
  {
    return unpack <value_cu> (val);
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
    return unpack <value_die> (val);
  }

  zw_value const *
  extract_dwarf (zw_value const *val, std::shared_ptr <dwfl_context> dwctx,
		 zw_error **out_err)
  {
    return capture_errors ([&] () {
	auto zwdw = std::make_unique <zw_value>
	  (std::make_unique <value_dwarf> ("???", std::move (dwctx),
					   0, doneness::raw));
	auto ret = zwdw.get ();
	libzw_cache (*val).push_back (std::move (zwdw));
	return ret;
      }, nullptr, out_err);
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
  return extract_dwarf (val, die (val).get_dwctx (), out_err);
}

namespace
{
  value_attr const &
  attr (zw_value const *val)
  {
    return unpack <value_attr> (val);
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
  return extract_dwarf (val, attr (val).get_dwctx (), out_err);
}
