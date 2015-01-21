/* Parts of this file were adapted from eu-readelf.
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

#include <dwarf.h>
#include <stdexcept>
#include <iostream>
#include <climits>

#include "known-dwarf.h"
#include "constant.hh"
#include "flag_saver.hh"

static const char *
abbreviate (char const *name, size_t prefix_len, brevity brv)
{
  return name + (brv == brevity::full ? 0 : prefix_len);
}

static const char *
dwarf_tag_string (int tag, brevity brv)
{
  switch (tag)
    {
#define ONE_KNOWN_DW_TAG(NAME, CODE)					\
      case CODE: return abbreviate (#CODE, sizeof "DW_TAG", brv);
      ALL_KNOWN_DW_TAG
#undef ONE_KNOWN_DW_TAG
    default:
      return nullptr;
    }
}


static const char *
dwarf_attr_string (int attrnum, brevity brv)
{
  switch (attrnum)
    {
#define ONE_KNOWN_DW_AT(NAME, CODE)					\
      case CODE: return abbreviate (#CODE, sizeof "DW_AT", brv);
      ALL_KNOWN_DW_AT
#undef ONE_KNOWN_DW_AT
    default:
      return nullptr;
    }
}


static const char *
dwarf_form_string (int form, brevity brv)
{
  switch (form)
    {
#define ONE_KNOWN_DW_FORM_DESC(NAME, CODE, DESC) ONE_KNOWN_DW_FORM (NAME, CODE)
#define ONE_KNOWN_DW_FORM(NAME, CODE)					\
      case CODE: return abbreviate (#CODE, sizeof "DW_FORM", brv);
      ALL_KNOWN_DW_FORM
#undef ONE_KNOWN_DW_FORM
#undef ONE_KNOWN_DW_FORM_DESC
    default:
      return nullptr;
    }
}


static const char *
dwarf_lang_string (int lang, brevity brv)
{
  switch (lang)
    {
#define ONE_KNOWN_DW_LANG_DESC(NAME, CODE, DESC)			\
      case CODE: return abbreviate (#CODE, sizeof "DW_LANG", brv);
      ALL_KNOWN_DW_LANG
#undef ONE_KNOWN_DW_LANG_DESC
    default:
      return nullptr;
    }
}


static const char *
dwarf_macinfo_string (int code, brevity brv)
{
  switch (code)
    {
#define ONE_KNOWN_DW_MACINFO(NAME, CODE)				\
      case CODE: return abbreviate (#CODE, sizeof "DW_MACINFO", brv);
      ALL_KNOWN_DW_MACINFO
#undef ONE_KNOWN_DW_MACINFO
    default:
      return nullptr;
    }
}


static const char *
dwarf_macro_string (int code, brevity brv)
{
  switch (code)
    {
#define ONE_KNOWN_DW_MACRO_GNU(NAME, CODE)				\
      /* N.B. don't snip the GNU_ part, that belongs to the constant,	\
	 not to the domain.  */						\
    case CODE: return abbreviate (#CODE, sizeof "DW_MACRO", brv);
      ALL_KNOWN_DW_MACRO_GNU
#undef ONE_KNOWN_DW_MACRO_GNU
    default:
      return nullptr;
    }
}


static const char *
dwarf_inline_string (int code, brevity brv)
{
  switch (code)
    {
#define ONE_KNOWN_DW_INL(NAME, CODE)					\
      case CODE: return abbreviate (#CODE, sizeof "DW_INL", brv);
      ALL_KNOWN_DW_INL
#undef ONE_KNOWN_DW_INL
    default:
      return nullptr;
    }
}


static const char *
dwarf_encoding_string (int code, brevity brv)
{
  switch (code)
    {
#define ONE_KNOWN_DW_ATE(NAME, CODE)					\
      case CODE: return abbreviate (#CODE, sizeof "DW_ATE", brv);
      ALL_KNOWN_DW_ATE
#undef ONE_KNOWN_DW_ATE
    default:
      return nullptr;
    }
}


static const char *
dwarf_access_string (int code, brevity brv)
{
  switch (code)
    {
#define ONE_KNOWN_DW_ACCESS(NAME, CODE)					\
      case CODE: return abbreviate (#CODE, sizeof "DW_ACCESS", brv);
      ALL_KNOWN_DW_ACCESS
#undef ONE_KNOWN_DW_ACCESS
    default:
      return nullptr;
    }
}


static const char *
dwarf_visibility_string (int code, brevity brv)
{
  switch (code)
    {
#define ONE_KNOWN_DW_VIS(NAME, CODE)					\
      case CODE: return abbreviate (#CODE, sizeof "DW_VIS", brv);
      ALL_KNOWN_DW_VIS
#undef ONE_KNOWN_DW_VIS
    default:
      return nullptr;
    }
}


static const char *
dwarf_virtuality_string (int code, brevity brv)
{
  switch (code)
    {
#define ONE_KNOWN_DW_VIRTUALITY(NAME, CODE)				\
      case CODE: return abbreviate (#CODE, sizeof "DW_VIRTUALITY", brv);
      ALL_KNOWN_DW_VIRTUALITY
#undef ONE_KNOWN_DW_VIRTUALITY
    default:
      return nullptr;
    }
}


static const char *
dwarf_identifier_case_string (int code, brevity brv)
{
  switch (code)
    {
#define ONE_KNOWN_DW_ID(NAME, CODE)					\
      case CODE: return abbreviate (#CODE, sizeof "DW_ID", brv);
      ALL_KNOWN_DW_ID
#undef ONE_KNOWN_DW_ID
    default:
      return nullptr;
    }
}


static const char *
dwarf_calling_convention_string (int code, brevity brv)
{
  switch (code)
    {
#define ONE_KNOWN_DW_CC(NAME, CODE)					\
      case CODE: return abbreviate (#CODE, sizeof "DW_CC", brv);
      ALL_KNOWN_DW_CC
#undef ONE_KNOWN_DW_CC
    default:
      return nullptr;
    }
}


static const char *
dwarf_ordering_string (int code, brevity brv)
{
  switch (code)
    {
#define ONE_KNOWN_DW_ORD(NAME, CODE)					\
      case CODE: return abbreviate (#CODE, sizeof "DW_ORD", brv);
      ALL_KNOWN_DW_ORD
#undef ONE_KNOWN_DW_ORD
    default:
      return nullptr;
    }
}


static const char *
dwarf_discr_list_string (int code, brevity brv)
{
  switch (code)
    {
#define ONE_KNOWN_DW_DSC(NAME, CODE)					\
      case CODE: return abbreviate (#CODE, sizeof "DW_DSC", brv);
      ALL_KNOWN_DW_DSC
#undef ONE_KNOWN_DW_DSC
    default:
      return nullptr;
    }
}


static const char *
dwarf_decimal_sign_string (int code, brevity brv)
{
  switch (code)
    {
#define ONE_KNOWN_DW_DS(NAME, CODE)					\
      case CODE: return abbreviate (#CODE, sizeof "DW_DS", brv);
      ALL_KNOWN_DW_DS
#undef ONE_KNOWN_DW_DS
    default:
      return nullptr;
    }
}


static const char *
dwarf_locexpr_opcode_string (int code, brevity brv)
{
  switch (code)
    {
#define ONE_KNOWN_DW_OP_DESC(NAME, CODE, DESC) ONE_KNOWN_DW_OP (NAME, CODE)
#define ONE_KNOWN_DW_OP(NAME, CODE)					\
      case CODE: return abbreviate (#CODE, sizeof "DW_OP", brv);
      ALL_KNOWN_DW_OP
#undef ONE_KNOWN_DW_OP
#undef ONE_KNOWN_DW_OP_DESC
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
#define ONE_KNOWN_DW_END(NAME, CODE)					\
      case CODE: return abbreviate (#CODE, sizeof "DW_END", brv);
      ALL_KNOWN_DW_END
#undef ONE_KNOWN_DW_END
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

namespace
{
  struct dw_simple_dom
    : public constant_dom
  {
    char const *m_name;
    char const *(*m_stringer) (int, brevity);
    int m_low_user;
    int m_high_user;
    bool m_print_unknown;

    dw_simple_dom (char const *name, char const *(*stringer) (int, brevity),
		   int low_user, int high_user, bool print_unknown)
      : m_name {name}
      , m_stringer {stringer}
      , m_low_user {low_user}
      , m_high_user {high_user}
      , m_print_unknown {print_unknown}
    {}

    void
    show (mpz_class const &v, std::ostream &o, brevity brv) const override
    {
      int code = positive_int_from_mpz (v);
      const char *ret = m_stringer (code, brv);
      o << string_or_unknown (ret, m_name, brv, code,
			      m_low_user, m_high_user, m_print_unknown);
    }

    char const *name () const override
    {
      return m_name;
    }
  };
}

constant_dom const &
dw_tag_dom ()
{
  static dw_simple_dom dom {"DW_TAG_", dwarf_tag_string,
			    DW_TAG_lo_user, DW_TAG_hi_user, true};
  return dom;
}

constant_dom const &
dw_attr_dom ()
{
  static dw_simple_dom dom {"DW_AT_", dwarf_attr_string,
			    DW_AT_lo_user, DW_AT_hi_user, true};
  return dom;
}

constant_dom const &
dw_form_dom ()
{
  static dw_simple_dom dom {"DW_FORM_", dwarf_form_string,
			    0, 0, true};
  return dom;
}

constant_dom const &
dw_lang_dom ()
{
  static dw_simple_dom dom {"DW_LANG_", dwarf_lang_string,
			    DW_LANG_lo_user, DW_LANG_hi_user, false};
  return dom;
}

constant_dom const &
dw_macinfo_dom ()
{
  static dw_simple_dom dom {"DW_MACINFO_", dwarf_macinfo_string,
			    0, 0, false};
  return dom;
}

constant_dom const &
dw_macro_dom ()
{
  static dw_simple_dom dom {"DW_MACRO_", dwarf_macro_string,
			    DW_MACRO_GNU_lo_user, DW_MACRO_GNU_hi_user, true};
  return dom;
}

constant_dom const &
dw_inline_dom ()
{
  static dw_simple_dom dom {"DW_INL_", dwarf_inline_string,
			    0, 0, false};
  return dom;
}

constant_dom const &
dw_encoding_dom ()
{
  static dw_simple_dom dom {"DW_ATE_", dwarf_encoding_string,
			    DW_ATE_lo_user, DW_ATE_hi_user, false};
  return dom;
}

constant_dom const &
dw_access_dom ()
{
  static dw_simple_dom dom {"DW_ACCESS_", dwarf_access_string,
			    0, 0, false};
  return dom;
}

constant_dom const &
dw_visibility_dom ()
{
  static dw_simple_dom dom {"DW_VIS_", dwarf_visibility_string,
			    0, 0, false};
  return dom;
}

constant_dom const &
dw_virtuality_dom ()
{
  static dw_simple_dom dom {"DW_VIRTUALITY_", dwarf_virtuality_string,
			    0, 0, false};
  return dom;
}

constant_dom const &
dw_identifier_case_dom ()
{
  static dw_simple_dom dom {"DW_ID_", dwarf_identifier_case_string,
			    0, 0, false};
  return dom;
}

constant_dom const &
dw_calling_convention_dom ()
{
  static dw_simple_dom dom {"DW_CC_", dwarf_calling_convention_string,
			    DW_CC_lo_user, DW_CC_hi_user, false};
  return dom;
}

constant_dom const &
dw_ordering_dom ()
{
  static dw_simple_dom dom {"DW_ORD_", dwarf_ordering_string,
			    0, 0, false};
  return dom;
}

constant_dom const &
dw_discr_list_dom ()
{
  static dw_simple_dom dom {"DW_DSC_", dwarf_discr_list_string,
			    0, 0, false};
  return dom;
}

constant_dom const &
dw_decimal_sign_dom ()
{
  static dw_simple_dom dom {"DW_DS_", dwarf_decimal_sign_string,
			    0, 0, false};
  return dom;
}

constant_dom const &
dw_locexpr_opcode_dom ()
{
  static dw_simple_dom dom {"DW_OP_", dwarf_locexpr_opcode_string,
			    DW_OP_lo_user, DW_OP_hi_user, true};
  return dom;
}

constant_dom const &
dw_address_class_dom ()
{
  static dw_simple_dom dom {"DW_ADDR_", dwarf_address_class_string,
			    0, 0, false};
  return dom;
}

constant_dom const &
dw_endianity_dom ()
{
  static dw_simple_dom dom {"DW_END_", dwarf_endianity_string,
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

constant_dom const &
dw_address_dom ()
{
  static dw_hex_constant_dom_t dw_address_dom_obj ("Dwarf_Address");
  return dw_address_dom_obj;
}

constant_dom const &
dw_offset_dom ()
{
  static dw_hex_constant_dom_t dw_offset_dom_obj ("Dwarf_Off");
  return dw_offset_dom_obj;
}

constant_dom const &
dw_abbrevcode_dom ()
{
  static dw_dec_constant_dom_t dw_abbrevcode_dom_obj ("Dwarf_Abbrev code");
  return dw_abbrevcode_dom_obj;
}
