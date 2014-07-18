/* Parts of this file were adapted from eu-readelf.
   Copyright (C) 1999-2014 Red Hat, Inc.

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

#include "known-dwarf.h"
#include "constant.hh"

static const char *
dwarf_tag_string (unsigned int tag, bool shrt)
{
  switch (tag)
    {
#define ONE_KNOWN_DW_TAG(NAME, CODE)			\
	case CODE: return #CODE + (shrt ? 7 : 0);
      ALL_KNOWN_DW_TAG
#undef ONE_KNOWN_DW_TAG
    default:
      return nullptr;
    }
}


static const char *
dwarf_attr_string (int attrnum, bool shrt)
{
  switch (attrnum)
    {
#define ONE_KNOWN_DW_AT(NAME, CODE) \
	case CODE: return #CODE + (shrt ? 6 : 0);
      ALL_KNOWN_DW_AT
#undef ONE_KNOWN_DW_AT
    default:
      return nullptr;
    }
}


static const char *
dwarf_form_string (int form, bool shrt)
{
  switch (form)
    {
#define ONE_KNOWN_DW_FORM_DESC(NAME, CODE, DESC) ONE_KNOWN_DW_FORM (NAME, CODE)
#define ONE_KNOWN_DW_FORM(NAME, CODE) \
	case CODE: return #CODE + (shrt ? 8 : 0);
      ALL_KNOWN_DW_FORM
#undef ONE_KNOWN_DW_FORM
#undef ONE_KNOWN_DW_FORM_DESC
    default:
      return nullptr;
    }
}


static const char *
dwarf_lang_string (int lang)
{
  switch (lang)
    {
#define ONE_KNOWN_DW_LANG_DESC(NAME, CODE, DESC) case CODE: return #CODE;
      ALL_KNOWN_DW_LANG
#undef ONE_KNOWN_DW_LANG_DESC
    default:
      return nullptr;
    }
}


static const char *
dwarf_inline_string (int code)
{
  switch (code)
    {
#define ONE_KNOWN_DW_INL(NAME, CODE) case CODE: return #CODE;
      ALL_KNOWN_DW_INL
#undef ONE_KNOWN_DW_INL
    default:
      return nullptr;
    }
}


static const char *
dwarf_encoding_string (int code)
{
  switch (code)
    {
#define ONE_KNOWN_DW_ATE(NAME, CODE) case CODE: return #CODE;
      ALL_KNOWN_DW_ATE
#undef ONE_KNOWN_DW_ATE
    default:
      return nullptr;
    }
}


static const char *
dwarf_access_string (int code)
{
  switch (code)
    {
#define ONE_KNOWN_DW_ACCESS(NAME, CODE) case CODE: return #CODE;
      ALL_KNOWN_DW_ACCESS
#undef ONE_KNOWN_DW_ACCESS
    default:
      return nullptr;
    }
}


static const char *
dwarf_visibility_string (int code)
{
  switch (code)
    {
#define ONE_KNOWN_DW_VIS(NAME, CODE) case CODE: return #CODE;
      ALL_KNOWN_DW_VIS
#undef ONE_KNOWN_DW_VIS
    default:
      return nullptr;
    }
}


static const char *
dwarf_virtuality_string (int code)
{
  switch (code)
    {
#define ONE_KNOWN_DW_VIRTUALITY(NAME, CODE) case CODE: return #CODE;
      ALL_KNOWN_DW_VIRTUALITY
#undef ONE_KNOWN_DW_VIRTUALITY
    default:
      return nullptr;
    }
}


static const char *
dwarf_identifier_case_string (int code)
{
  switch (code)
    {
#define ONE_KNOWN_DW_ID(NAME, CODE) case CODE: return #CODE;
      ALL_KNOWN_DW_ID
#undef ONE_KNOWN_DW_ID
    default:
      return nullptr;
    }
}


static const char *
dwarf_calling_convention_string (int code)
{
  switch (code)
    {
#define ONE_KNOWN_DW_CC(NAME, CODE) case CODE: return #CODE;
      ALL_KNOWN_DW_CC
#undef ONE_KNOWN_DW_CC
    default:
      return nullptr;
    }
}


static const char *
dwarf_ordering_string (int code)
{
  switch (code)
    {
#define ONE_KNOWN_DW_ORD(NAME, CODE) case CODE: return #CODE;
      ALL_KNOWN_DW_ORD
#undef ONE_KNOWN_DW_ORD
    default:
      return nullptr;
    }
}


static const char *
dwarf_discr_list_string (int code)
{
  switch (code)
    {
#define ONE_KNOWN_DW_DSC(NAME, CODE) case CODE: return #CODE;
      ALL_KNOWN_DW_DSC
#undef ONE_KNOWN_DW_DSC
    default:
      return nullptr;
    }
}


static const char *
dwarf_decimal_sign_string (int code)
{
  switch (code)
    {
#define ONE_KNOWN_DW_DS(NAME, CODE) case CODE: return #CODE;
      ALL_KNOWN_DW_DS
#undef ONE_KNOWN_DW_DS
    default:
      return nullptr;
    }
}


static const char *
dwarf_locexpr_opcode_string (int code, bool shrt)
{
  switch (code)
    {
#define ONE_KNOWN_DW_OP_DESC(NAME, CODE, DESC) ONE_KNOWN_DW_OP (NAME, CODE)
#define ONE_KNOWN_DW_OP(NAME, CODE) case CODE: return #CODE + (shrt ? 6 : 0);
      ALL_KNOWN_DW_OP
#undef ONE_KNOWN_DW_OP
#undef ONE_KNOWN_DW_OP_DESC
    default:
      return nullptr;
    }
}


static const char *
dwarf_address_class_string (int code)
{
  switch (code)
    {
    case DW_ADDR_none:
      return "DW_ADDR_none";
    default:
      return nullptr;
    }
}


static const char *
dwarf_endianity_string (int code)
{
  switch (code)
    {
#define ONE_KNOWN_DW_END(NAME, CODE) case CODE: return #CODE;
      ALL_KNOWN_DW_END
#undef ONE_KNOWN_DW_END
    default:
      return nullptr;
    }
}


/* Used by all dwarf_foo_name functions.  */
static const char *
string_or_unknown (const char *known, const char *prefix,
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
      snprintf (unknown_buf, sizeof unknown_buf, "%s??? (%#x)", prefix, code);
      return unknown_buf;
    }

  return "???";
}

namespace
{
  int
  positive_int_from_mpz (mpz_class const &v)
  {
    if (! v.fits_uint_p ())
      return -1;
    unsigned long ul = v.get_ui ();
    if (ul >= (1U << 31))
      return -1;
    return static_cast <int> (ul);
  }
}

static class dw_tag_dom_t
  : public constant_dom
{
  bool m_short;

public:
  dw_tag_dom_t (bool shrt)
    : m_short {shrt}
  {}

  void
  show (mpz_class const &v, std::ostream &o) const override
  {
    int tag = positive_int_from_mpz (v);
    const char *ret = dwarf_tag_string (tag, m_short);
    o << string_or_unknown (ret, "DW_TAG_",
			    tag, DW_TAG_lo_user, DW_TAG_hi_user, true);
  }

  std::string name () const override
  {
    return "DW_TAG_*";
  }
} dw_tag_dom_obj {false}, dw_tag_short_dom_obj {true};

constant_dom const &dw_tag_dom = dw_tag_dom_obj;
constant_dom const &dw_tag_short_dom = dw_tag_short_dom_obj;


static class dw_attr_dom_t
  : public constant_dom
{
  bool m_short;

public:
  dw_attr_dom_t (bool shrt)
    : m_short {shrt}
  {}

  virtual void
  show (mpz_class const &v, std::ostream &o) const
  {
    int attr = positive_int_from_mpz (v);
    const char *ret = dwarf_attr_string (attr, m_short);
    o << string_or_unknown (ret, "DW_AT_",
			    attr, DW_AT_lo_user, DW_AT_hi_user, true);
  }

  std::string name () const override
  {
    return "DW_ATTR_*";
  }
} dw_attr_dom_obj {false}, dw_attr_short_dom_obj {true};

constant_dom const &dw_attr_dom = dw_attr_dom_obj;
constant_dom const &dw_attr_short_dom = dw_attr_short_dom_obj;


static struct dw_form_dom_t
  : public constant_dom
{
  bool m_short;

public:
  dw_form_dom_t (bool shrt)
    : m_short {shrt}
  {}

  virtual void
  show (mpz_class const &v, std::ostream &o) const
  {
    int form = positive_int_from_mpz (v);
    const char *ret = dwarf_form_string (form, m_short);
    o << string_or_unknown (ret, "DW_FORM_", form, 0, 0, true);
  }

  std::string name () const override
  {
    return "DW_FORM_*";
  }
} dw_form_dom_obj {false}, dw_form_short_dom_obj {true};

constant_dom const &dw_form_dom = dw_form_dom_obj;
constant_dom const &dw_form_short_dom = dw_form_short_dom_obj;


static struct
  : public constant_dom
{
  virtual void
  show (mpz_class const &v, std::ostream &o) const
  {
    int lang = positive_int_from_mpz (v);
    const char *ret = dwarf_lang_string (lang);
    o << string_or_unknown (ret, "DW_LANG_",
			    lang, DW_LANG_lo_user, DW_LANG_hi_user, false);
  }

  std::string name () const override
  {
    return "DW_LANG_*";
  }
} dw_lang_dom_obj;

constant_dom const &dw_lang_dom = dw_lang_dom_obj;


static struct
  : public constant_dom
{
  virtual void
  show (mpz_class const &v, std::ostream &o) const
  {
    int code = positive_int_from_mpz (v);
    const char *ret = dwarf_inline_string (code);
    o << string_or_unknown (ret, "DW_INL_", code, 0, 0, false);
  }

  std::string name () const override
  {
    return "DW_INL_*";
  }
} dw_inline_dom_obj;

constant_dom const &dw_inline_dom = dw_inline_dom_obj;


static struct
  : public constant_dom
{
  virtual void
  show (mpz_class const &v, std::ostream &o) const
  {
    int code = positive_int_from_mpz (v);
    const char *ret = dwarf_encoding_string (code);
    o << string_or_unknown (ret, "DW_ATE_",
			    code, DW_ATE_lo_user, DW_ATE_hi_user, false);
  }

  std::string name () const override
  {
    return "DW_ATE_*";
  }
} dw_encoding_dom_obj;

constant_dom const &dw_encoding_dom = dw_encoding_dom_obj;


static struct
  : public constant_dom
{
  virtual void
  show (mpz_class const &v, std::ostream &o) const
  {
    int code = positive_int_from_mpz (v);
    const char *ret = dwarf_access_string (code);
    o << string_or_unknown (ret, "DW_ACCESS_", code, 0, 0, false);
  }

  std::string name () const override
  {
    return "DW_ACCESS_*";
  }
} dw_access_dom_obj;

constant_dom const &dw_access_dom = dw_access_dom_obj;


static struct
  : public constant_dom
{
  virtual void
  show (mpz_class const &v, std::ostream &o) const
  {
    int code = positive_int_from_mpz (v);
    const char *ret = dwarf_visibility_string (code);
    o << string_or_unknown (ret, "DW_VIS_", code, 0, 0, false);
  }

  std::string name () const override
  {
    return "DW_VIS_*";
  }
} dw_visibility_dom_obj;

constant_dom const &dw_visibility_dom = dw_visibility_dom_obj;


static struct
  : public constant_dom
{
  virtual void
  show (mpz_class const &v, std::ostream &o) const
  {
    int code = positive_int_from_mpz (v);
    const char *ret = dwarf_virtuality_string (code);
    o << string_or_unknown (ret, "DW_VIRTUALITY_", code, 0, 0, false);
  }

  std::string name () const override
  {
    return "DW_VIRTUALITY_*";
  }
} dw_virtuality_dom_obj;

constant_dom const &dw_virtuality_dom = dw_virtuality_dom_obj;


static struct
  : public constant_dom
{
  virtual void
  show (mpz_class const &v, std::ostream &o) const
  {
    int code = positive_int_from_mpz (v);
    const char *ret = dwarf_identifier_case_string (code);
    o << string_or_unknown (ret, "DW_ID_", code, 0, 0, false);
  }

  std::string name () const override
  {
    return "DW_ID_*";
  }
} dw_identifier_case_dom_obj;

constant_dom const &dw_identifier_case_dom = dw_identifier_case_dom_obj;


static struct
  : public constant_dom
{
  virtual void
  show (mpz_class const &v, std::ostream &o) const
  {
    int code = positive_int_from_mpz (v);
    const char *ret = dwarf_calling_convention_string (code);
    o << string_or_unknown (ret, "DW_CC_",
			    code, DW_CC_lo_user, DW_CC_hi_user, false);
  }

  std::string name () const override
  {
    return "DW_CC_*";
  }
} dw_calling_convention_dom_obj;

constant_dom const &dw_calling_convention_dom = dw_calling_convention_dom_obj;


static struct
  : public constant_dom
{
  virtual void
  show (mpz_class const &v, std::ostream &o) const
  {
    int code = positive_int_from_mpz (v);
    const char *ret = dwarf_ordering_string (code);
    o << string_or_unknown (ret, "DW_ORD_", code, 0, 0, false);
  }

  std::string name () const override
  {
    return "DW_ORD_*";
  }
} dw_ordering_dom_obj;

constant_dom const &dw_ordering_dom = dw_ordering_dom_obj;


static struct
  : public constant_dom
{
  virtual void
  show (mpz_class const &v, std::ostream &o) const
  {
    int code = positive_int_from_mpz (v);
    const char *ret = dwarf_discr_list_string (code);
    o << string_or_unknown (ret, "DW_DSC_", code, 0, 0, false);
  }

  std::string name () const override
  {
    return "DW_DSC_*";
  }
} dw_discr_list_dom_obj;

constant_dom const &dw_discr_list_dom = dw_discr_list_dom_obj;


static struct
  : public constant_dom
{
  virtual void
  show (mpz_class const &v, std::ostream &o) const
  {
    int code = positive_int_from_mpz (v);
    const char *ret = dwarf_decimal_sign_string (code);
    o << string_or_unknown (ret, "DW_DS_", code, 0, 0, false);
  }

  std::string name () const override
  {
    return "DW_DS_*";
  }
} dw_decimal_sign_dom_obj;

constant_dom const &dw_decimal_sign_dom = dw_decimal_sign_dom_obj;


static struct dw_locexpr_op_t
  : public constant_dom
{
  bool m_short;

  explicit dw_locexpr_op_t (bool shrt)
    : m_short {shrt}
  {}

  virtual void
  show (mpz_class const &v, std::ostream &o) const
  {
    int code = positive_int_from_mpz (v);
    const char *ret = dwarf_locexpr_opcode_string (code, m_short);
    o << string_or_unknown (ret, "DW_OP_",
			    code, DW_OP_lo_user, DW_OP_hi_user, true);
  }

  std::string name () const override
  {
    return "DW_OP_*";
  }
} dw_locexpr_opcode_dom_obj {false}, dw_locexpr_opcode_short_dom_obj {true};

constant_dom const &dw_locexpr_opcode_dom = dw_locexpr_opcode_dom_obj;
constant_dom const &dw_locexpr_opcode_short_dom
	= dw_locexpr_opcode_short_dom_obj;


static struct
  : public constant_dom
{
  virtual void
  show (mpz_class const &v, std::ostream &o) const
  {
    int code = positive_int_from_mpz (v);
    const char *ret = dwarf_address_class_string (code);
    o << string_or_unknown (ret, "DW_ADDR_", code, 0, 0, false);
  }

  std::string name () const override
  {
    return "DW_ADDR_*";
  }
} dw_address_class_dom_obj;

constant_dom const &dw_address_class_dom = dw_address_class_dom_obj;


static struct
  : public constant_dom
{
  virtual void
  show (mpz_class const &v, std::ostream &o) const
  {
    int code = positive_int_from_mpz (v);
    const char *ret = dwarf_endianity_string (code);
    o << string_or_unknown (ret, "DW_END_", code, 0, 0, false);
  }

  std::string name () const override
  {
    return "DW_END_*";
  }
} dw_endianity_dom_obj;

constant_dom const &dw_endianity_dom = dw_endianity_dom_obj;
