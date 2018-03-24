/*
   Copyright (C) 2017, 2018 Petr Machata
   Parts of this file were adapted from eu-readelf.
   Copyright (C) 1999-2015 Red Hat, Inc.

   This file is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "dwcst.hh"

#include <dwarf.h>
#include <stdexcept>
#include <iostream>
#include <climits>

#include "known-dwarf.h"
#include "known-dwarf-macro-gnu.h"
#include "constant.hh"
#include "flag_saver.hh"

static const char *
dwarf_tag_string (int tag, brevity brv)
{
  switch (tag)
    {
#define DWARF_ONE_KNOWN_DW_TAG(NAME, CODE)				\
      case CODE: return abbreviate (#CODE, sizeof "DW_TAG", brv);
      DWARF_ALL_KNOWN_DW_TAG
#undef DWARF_ONE_KNOWN_DW_TAG
    default:
      return nullptr;
    }
}


static const char *
dwarf_attr_string (int attrnum, brevity brv)
{
  switch (attrnum)
    {
#define DWARF_ONE_KNOWN_DW_AT(NAME, CODE)				\
      case CODE: return abbreviate (#CODE, sizeof "DW_AT", brv);
      DWARF_ALL_KNOWN_DW_AT
#undef DWARF_ONE_KNOWN_DW_AT
    default:
      return nullptr;
    }
}


static const char *
dwarf_form_string (int form, brevity brv)
{
  switch (form)
    {
#define DWARF_ONE_KNOWN_DW_FORM(NAME, CODE)				\
      case CODE: return abbreviate (#CODE, sizeof "DW_FORM", brv);
      DWARF_ALL_KNOWN_DW_FORM
#undef ONE_KNOWN_DW_FORM
    default:
      return nullptr;
    }
}


static const char *
dwarf_lang_string (int lang, brevity brv)
{
  switch (lang)
    {
#define DWARF_ONE_KNOWN_DW_LANG(NAME, CODE)			\
      case CODE: return abbreviate (#CODE, sizeof "DW_LANG", brv);
      DWARF_ALL_KNOWN_DW_LANG
#undef DWARF_ONE_KNOWN_DW_LANG
    default:
      return nullptr;
    }
}


static const char *
dwarf_macinfo_string (int code, brevity brv)
{
  switch (code)
    {
#define DWARF_ONE_KNOWN_DW_MACINFO(NAME, CODE)				\
      case CODE: return abbreviate (#CODE, sizeof "DW_MACINFO", brv);
      DWARF_ALL_KNOWN_DW_MACINFO
#undef DWARF_ONE_KNOWN_DW_MACINFO
    default:
      return nullptr;
    }
}


static const char *
dwarf_macro_string (int code, brevity brv)
{
  switch (code)
    {
#define DWARF_ONE_KNOWN_DW_MACRO_GNU(NAME, CODE)			\
      /* N.B. don't snip the GNU_ part, that belongs to the constant,	\
	 not to the domain.  */						\
    case CODE: return abbreviate (#CODE, sizeof "DW_MACRO", brv);
      DWARF_ALL_KNOWN_DW_MACRO_GNU
#undef DWARF_ONE_KNOWN_DW_MACRO_GNU
    default:
      return nullptr;
    }
}


static const char *
dwarf_inline_string (int code, brevity brv)
{
  switch (code)
    {
#define DWARF_ONE_KNOWN_DW_INL(NAME, CODE)				\
      case CODE: return abbreviate (#CODE, sizeof "DW_INL", brv);
      DWARF_ALL_KNOWN_DW_INL
#undef DWARF_ONE_KNOWN_DW_INL
    default:
      return nullptr;
    }
}


static const char *
dwarf_encoding_string (int code, brevity brv)
{
  switch (code)
    {
#define DWARF_ONE_KNOWN_DW_ATE(NAME, CODE)				\
      case CODE: return abbreviate (#CODE, sizeof "DW_ATE", brv);
      DWARF_ALL_KNOWN_DW_ATE
#undef DWARF_ONE_KNOWN_DW_ATE
    default:
      return nullptr;
    }
}


static const char *
dwarf_access_string (int code, brevity brv)
{
  switch (code)
    {
#define DWARF_ONE_KNOWN_DW_ACCESS(NAME, CODE)				\
      case CODE: return abbreviate (#CODE, sizeof "DW_ACCESS", brv);
      DWARF_ALL_KNOWN_DW_ACCESS
#undef DWARF_ONE_KNOWN_DW_ACCESS
    default:
      return nullptr;
    }
}


static const char *
dwarf_visibility_string (int code, brevity brv)
{
  switch (code)
    {
#define DWARF_ONE_KNOWN_DW_VIS(NAME, CODE)				\
      case CODE: return abbreviate (#CODE, sizeof "DW_VIS", brv);
      DWARF_ALL_KNOWN_DW_VIS
#undef DWARF_ONE_KNOWN_DW_VIS
    default:
      return nullptr;
    }
}


static const char *
dwarf_virtuality_string (int code, brevity brv)
{
  switch (code)
    {
#define DWARF_ONE_KNOWN_DW_VIRTUALITY(NAME, CODE)			\
      case CODE: return abbreviate (#CODE, sizeof "DW_VIRTUALITY", brv);
      DWARF_ALL_KNOWN_DW_VIRTUALITY
#undef DWARF_ONE_KNOWN_DW_VIRTUALITY
    default:
      return nullptr;
    }
}


static const char *
dwarf_identifier_case_string (int code, brevity brv)
{
  switch (code)
    {
#define DWARF_ONE_KNOWN_DW_ID(NAME, CODE)				\
      case CODE: return abbreviate (#CODE, sizeof "DW_ID", brv);
      DWARF_ALL_KNOWN_DW_ID
#undef DWARF_ONE_KNOWN_DW_ID
    default:
      return nullptr;
    }
}


static const char *
dwarf_calling_convention_string (int code, brevity brv)
{
  switch (code)
    {
#define DWARF_ONE_KNOWN_DW_CC(NAME, CODE)				\
      case CODE: return abbreviate (#CODE, sizeof "DW_CC", brv);
      DWARF_ALL_KNOWN_DW_CC
#undef DWARF_ONE_KNOWN_DW_CC
    default:
      return nullptr;
    }
}


static const char *
dwarf_ordering_string (int code, brevity brv)
{
  switch (code)
    {
#define DWARF_ONE_KNOWN_DW_ORD(NAME, CODE)				\
      case CODE: return abbreviate (#CODE, sizeof "DW_ORD", brv);
      DWARF_ALL_KNOWN_DW_ORD
#undef DWARF_ONE_KNOWN_DW_ORD
    default:
      return nullptr;
    }
}


static const char *
dwarf_discr_list_string (int code, brevity brv)
{
  switch (code)
    {
#define DWARF_ONE_KNOWN_DW_DSC(NAME, CODE)				\
      case CODE: return abbreviate (#CODE, sizeof "DW_DSC", brv);
      DWARF_ALL_KNOWN_DW_DSC
#undef DWARF_ONE_KNOWN_DW_DSC
    default:
      return nullptr;
    }
}


static const char *
dwarf_decimal_sign_string (int code, brevity brv)
{
  switch (code)
    {
#define DWARF_ONE_KNOWN_DW_DS(NAME, CODE)				\
      case CODE: return abbreviate (#CODE, sizeof "DW_DS", brv);
      DWARF_ALL_KNOWN_DW_DS
#undef DWARF_ONE_KNOWN_DW_DS
    default:
      return nullptr;
    }
}


static const char *
dwarf_locexpr_opcode_string (int code, brevity brv)
{
  switch (code)
    {
#define DWARF_ONE_KNOWN_DW_OP(NAME, CODE)				\
      case CODE: return abbreviate (#CODE, sizeof "DW_OP", brv);
      DWARF_ALL_KNOWN_DW_OP
#undef DWARF_ONE_KNOWN_DW_OP
    default:
      return nullptr;
    }
}


static const char *
dwarf_address_class_string (int code, brevity brv)
{
  switch (code)
    {
    case DW_ADDR_none:
      return abbreviate ("DW_ADDR_none", sizeof "DW_ADDR", brv);
    default:
      return nullptr;
    }
}


static const char *
dwarf_endianity_string (int code, brevity brv)
{
  switch (code)
    {
#define DWARF_ONE_KNOWN_DW_END(NAME, CODE)				\
      case CODE: return abbreviate (#CODE, sizeof "DW_END", brv);
      DWARF_ALL_KNOWN_DW_END
#undef DWARF_ONE_KNOWN_DW_END
    default:
      return nullptr;
    }
}


static const char *
dwarf_defaulted_string (int code, brevity brv)
{
  switch (code)
    {
#define DWARF_ONE_KNOWN_DW_DEFAULTED(NAME, CODE)				\
      case CODE: return abbreviate (#CODE, sizeof "DW_DEFAULTED", brv);
      DWARF_ALL_KNOWN_DW_DEFAULTED
#undef DWARF_ONE_KNOWN_DW_DEFAULTED
    default:
      return nullptr;
    }
}


static const char *
string_or_unknown (const char *known, const char *prefix, brevity brv,
		   unsigned int code,
                   unsigned int lo_user, unsigned int hi_user,
		   bool print_unknown_num)
{
  static char unknown_buf[40];

  if (known != nullptr)
    return known;

  if (lo_user != 0 && code >= lo_user && code <= hi_user)
    {
      snprintf (unknown_buf, sizeof unknown_buf, "%slo_user+%#x",
		prefix, code - lo_user);
      return unknown_buf;
    }

  if (print_unknown_num)
    {
      snprintf (unknown_buf, sizeof unknown_buf,
		"%s??? (%#x)", brv == brevity::full ? prefix : "", code);
      return unknown_buf;
    }

  return "???";
}

namespace
{
  int
  positive_int_from_mpz (mpz_class const &v)
  {
    if (v < 0 || v.uval () > INT_MAX)
      return -1;
    return v.uval ();
  }
}

void
dw_simple_dom::show (mpz_class const &v, std::ostream &o, brevity brv) const
{
  int code = positive_int_from_mpz (v);
  const char *ret = m_stringer (code, brv);
  o << string_or_unknown (ret, m_name, brv, code,
			  m_low_user, m_high_user, m_print_unknown);
}

char const *
dw_simple_dom::name () const
{
  return m_name;
}

char const *
dw_simple_dom::docstring () const
{
  return
R"docstring(

These words push to the stack the Dwarf constants referenced by their
name.  Individual classes of constants (e.g. ``DW_TAG_``, ``DW_AT_``,
...) have distinct domains.  That means that, say, ``DW_TAG_member``
and ``DW_AT_bit_size`` don't compare equal, even though they both have
the same underlying value.  It is possible to extract that numeric
value using the word ``value``::

	$ dwgrep -c -e 'DW_TAG_member == DW_AT_bit_size'
	0
	$ dwgrep -c -e 'DW_TAG_member value == DW_AT_bit_size value'
	1

)docstring";
}


zw_cdom const &
dw_tag_dom ()
{
  static dw_simple_dom dom {"DW_TAG_", dwarf_tag_string,
			    DW_TAG_lo_user, DW_TAG_hi_user, true};
  return dom;
}

zw_cdom const &
dw_attr_dom ()
{
  static dw_simple_dom dom {"DW_AT_", dwarf_attr_string,
			    DW_AT_lo_user, DW_AT_hi_user, true};
  return dom;
}

zw_cdom const &
dw_form_dom ()
{
  static dw_simple_dom dom {"DW_FORM_", dwarf_form_string,
			    0, 0, true};
  return dom;
}

zw_cdom const &
dw_lang_dom ()
{
  static dw_simple_dom dom {"DW_LANG_", dwarf_lang_string,
			    DW_LANG_lo_user, DW_LANG_hi_user, false};
  return dom;
}

zw_cdom const &
dw_macinfo_dom ()
{
  static dw_simple_dom dom {"DW_MACINFO_", dwarf_macinfo_string,
			    0, 0, false};
  return dom;
}

zw_cdom const &
dw_macro_dom ()
{
  static dw_simple_dom dom {"DW_MACRO_", dwarf_macro_string,
			    DW_MACRO_GNU_lo_user, DW_MACRO_GNU_hi_user, true};
  return dom;
}

zw_cdom const &
dw_inline_dom ()
{
  static dw_simple_dom dom {"DW_INL_", dwarf_inline_string,
			    0, 0, false};
  return dom;
}

zw_cdom const &
dw_encoding_dom ()
{
  static dw_simple_dom dom {"DW_ATE_", dwarf_encoding_string,
			    DW_ATE_lo_user, DW_ATE_hi_user, false};
  return dom;
}

zw_cdom const &
dw_access_dom ()
{
  static dw_simple_dom dom {"DW_ACCESS_", dwarf_access_string,
			    0, 0, false};
  return dom;
}

zw_cdom const &
dw_visibility_dom ()
{
  static dw_simple_dom dom {"DW_VIS_", dwarf_visibility_string,
			    0, 0, false};
  return dom;
}

zw_cdom const &
dw_virtuality_dom ()
{
  static dw_simple_dom dom {"DW_VIRTUALITY_", dwarf_virtuality_string,
			    0, 0, false};
  return dom;
}

zw_cdom const &
dw_identifier_case_dom ()
{
  static dw_simple_dom dom {"DW_ID_", dwarf_identifier_case_string,
			    0, 0, false};
  return dom;
}

zw_cdom const &
dw_calling_convention_dom ()
{
  static dw_simple_dom dom {"DW_CC_", dwarf_calling_convention_string,
			    DW_CC_lo_user, DW_CC_hi_user, false};
  return dom;
}

zw_cdom const &
dw_ordering_dom ()
{
  static dw_simple_dom dom {"DW_ORD_", dwarf_ordering_string,
			    0, 0, false};
  return dom;
}

zw_cdom const &
dw_discr_list_dom ()
{
  static dw_simple_dom dom {"DW_DSC_", dwarf_discr_list_string,
			    0, 0, false};
  return dom;
}

zw_cdom const &
dw_decimal_sign_dom ()
{
  static dw_simple_dom dom {"DW_DS_", dwarf_decimal_sign_string,
			    0, 0, false};
  return dom;
}

zw_cdom const &
dw_locexpr_opcode_dom ()
{
  static dw_simple_dom dom {"DW_OP_", dwarf_locexpr_opcode_string,
			    DW_OP_lo_user, DW_OP_hi_user, true};
  return dom;
}

zw_cdom const &
dw_address_class_dom ()
{
  static dw_simple_dom dom {"DW_ADDR_", dwarf_address_class_string,
			    0, 0, false};
  return dom;
}

zw_cdom const &
dw_endianity_dom ()
{
  static dw_simple_dom dom {"DW_END_", dwarf_endianity_string,
			    0, 0, false};
  return dom;
}

zw_cdom const &
dw_defaulted_dom ()
{
  static dw_simple_dom dom {"DW_DEFAULTED_", dwarf_defaulted_string,
			    0, 0, false};
  return dom;
}

namespace
{
  struct dw_hex_constant_dom_t
    : public numeric_constant_dom_t
  {
    using numeric_constant_dom_t::numeric_constant_dom_t;

    void
    show (mpz_class const &v, std::ostream &o, brevity brv) const override
    {
      ios_flag_saver s {o};
      o << std::hex << std::showbase << v;
    }
  };

  struct dw_dec_constant_dom_t
    : public numeric_constant_dom_t
  {
    using numeric_constant_dom_t::numeric_constant_dom_t;

    void
    show (mpz_class const &v, std::ostream &o, brevity brv) const override
    {
      ios_flag_saver s {o};
      o << std::dec << std::showbase << v;
    }
  };
}

zw_cdom const &
dw_address_dom ()
{
  static dw_hex_constant_dom_t dw_address_dom_obj ("Dwarf_Address");
  return dw_address_dom_obj;
}

zw_cdom const &
dw_offset_dom ()
{
  static dw_hex_constant_dom_t dw_offset_dom_obj ("Dwarf_Off");
  return dw_offset_dom_obj;
}

zw_cdom const &
dw_abbrevcode_dom ()
{
  static dw_dec_constant_dom_t dw_abbrevcode_dom_obj ("Dwarf_Abbrev code");
  return dw_abbrevcode_dom_obj;
}
