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
dwarf_tag_string (unsigned int tag)
{
  switch (tag)
    {
#define ONE_KNOWN_DW_TAG(NAME, CODE) case CODE: return #CODE;
      ALL_KNOWN_DW_TAG
#undef ONE_KNOWN_DW_TAG
    default:
      return nullptr;
    }
}

static unsigned int
dwarf_tag_value (std::string str)
{
  // XXX: This is very crude implementation, but I very much doubt
  // parsing performance will be any kind of bottleneck ever, so just
  // do the simplest thing that works.
#define ONE_KNOWN_DW_TAG(NAME, CODE) if (str == #CODE) return CODE;
  ALL_KNOWN_DW_TAG
#undef ONE_KNOWN_DW_TAG

    throw std::runtime_error (std::string ("Unknown tag: ") + str);
}


static const char *
dwarf_attr_string (unsigned int attrnum)
{
  switch (attrnum)
    {
#define ONE_KNOWN_DW_AT(NAME, CODE) case CODE: return #CODE;
      ALL_KNOWN_DW_AT
#undef ONE_KNOWN_DW_AT
    default:
      return nullptr;
    }
}

static unsigned int
dwarf_attr_value (std::string str)
{
#define ONE_KNOWN_DW_AT(NAME, CODE) if (str == #CODE) return CODE;
  ALL_KNOWN_DW_AT
#undef ONE_KNOWN_DW_AT

    throw std::runtime_error (std::string ("Unknown attribute: ") + str);
}


static const char *
dwarf_form_string (unsigned int form)
{
  switch (form)
    {
#define ONE_KNOWN_DW_FORM_DESC(NAME, CODE, DESC) ONE_KNOWN_DW_FORM (NAME, CODE)
#define ONE_KNOWN_DW_FORM(NAME, CODE) case CODE: return #CODE;
      ALL_KNOWN_DW_FORM
#undef ONE_KNOWN_DW_FORM
#undef ONE_KNOWN_DW_FORM_DESC
    default:
      return nullptr;
    }
}

static unsigned int
dwarf_form_value (std::string str)
{
#define ONE_KNOWN_DW_FORM_DESC(NAME, CODE, DESC) ONE_KNOWN_DW_FORM (NAME, CODE)
#define ONE_KNOWN_DW_FORM(NAME, CODE) if (str == #CODE) return CODE;
      ALL_KNOWN_DW_FORM
#undef ONE_KNOWN_DW_FORM
#undef ONE_KNOWN_DW_FORM_DESC

    throw std::runtime_error (std::string ("Unknown form value: ") + str);
}


static const char *
dwarf_lang_string (unsigned int lang)
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

static unsigned int
dwarf_lang_value (std::string str)
{
#define ONE_KNOWN_DW_LANG_DESC(NAME, CODE, DESC) if (str == #CODE) return CODE;
      ALL_KNOWN_DW_LANG
#undef ONE_KNOWN_DW_LANG_DESC

  throw std::runtime_error (std::string ("Unknown lang: ") + str);
}


static const char *
dwarf_inline_string (unsigned int code)
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

static unsigned int
dwarf_inline_value (std::string str)
{
#define ONE_KNOWN_DW_INL(NAME, CODE) if (str == #CODE) return CODE;
      ALL_KNOWN_DW_INL
#undef ONE_KNOWN_DW_INL
  throw std::runtime_error (std::string ("Unknown inline: ") + str);
}


static const char *
dwarf_encoding_string (unsigned int code)
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

static unsigned int
dwarf_encoding_value (std::string str)
{
#define ONE_KNOWN_DW_ATE(NAME, CODE) if (str == #CODE) return CODE;
      ALL_KNOWN_DW_ATE
#undef ONE_KNOWN_DW_ATE

  throw std::runtime_error (std::string ("Unknown encoding: ") + str);
}


static const char *
dwarf_access_string (unsigned int code)
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

static unsigned int
dwarf_access_value (std::string str)
{
#define ONE_KNOWN_DW_ACCESS(NAME, CODE) if (str == #CODE) return CODE;
      ALL_KNOWN_DW_ACCESS
#undef ONE_KNOWN_DW_ACCESS

  throw std::runtime_error (std::string ("Unknown access: ") + str);
}


static const char *
dwarf_visibility_string (unsigned int code)
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

static unsigned int
dwarf_visibility_value (std::string str)
{
#define ONE_KNOWN_DW_VIS(NAME, CODE) if (str == #CODE) return CODE;
      ALL_KNOWN_DW_VIS
#undef ONE_KNOWN_DW_VIS

  throw std::runtime_error (std::string ("Unknown visibility: ") + str);
}


static const char *
dwarf_virtuality_string (unsigned int code)
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

static unsigned int
dwarf_virtuality_value (std::string str)
{
#define ONE_KNOWN_DW_VIRTUALITY(NAME, CODE) if (str == #CODE) return CODE;
      ALL_KNOWN_DW_VIRTUALITY
#undef ONE_KNOWN_DW_VIRTUALITY

  throw std::runtime_error (std::string ("Unknown virtuality: ") + str);
}


static const char *
dwarf_identifier_case_string (unsigned int code)
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

static unsigned int
dwarf_identifier_case_value (std::string str)
{
#define ONE_KNOWN_DW_ID(NAME, CODE) if (str == #CODE) return CODE;
      ALL_KNOWN_DW_ID
#undef ONE_KNOWN_DW_ID

  throw std::runtime_error (std::string ("Unknown identifier case: ") + str);
}


static const char *
dwarf_calling_convention_string (unsigned int code)
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

static unsigned int
dwarf_calling_convention_value (std::string str)
{
#define ONE_KNOWN_DW_CC(NAME, CODE) if (str == #CODE) return CODE;
      ALL_KNOWN_DW_CC
#undef ONE_KNOWN_DW_CC

  throw std::runtime_error (std::string ("Unknown calling convention: ") + str);
}


static const char *
dwarf_ordering_string (unsigned int code)
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

static unsigned int
dwarf_ordering_value (std::string str)
{
#define ONE_KNOWN_DW_ORD(NAME, CODE) if (str == #CODE) return CODE;
      ALL_KNOWN_DW_ORD
#undef ONE_KNOWN_DW_ORD

  throw std::runtime_error (std::string ("Unknown ordering: ") + str);
}


static const char *
dwarf_discr_list_string (unsigned int code)
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

static unsigned int
dwarf_discr_list_value (std::string str)
{
#define ONE_KNOWN_DW_DSC(NAME, CODE) if (str == #CODE) return CODE;
      ALL_KNOWN_DW_DSC
#undef ONE_KNOWN_DW_DSC

  throw std::runtime_error (std::string ("Unknown discr list: ") + str);
}


static const char *
dwarf_locexpr_opcode_string (unsigned int code)
{
  switch (code)
    {
#define ONE_KNOWN_DW_OP_DESC(NAME, CODE, DESC) ONE_KNOWN_DW_OP (NAME, CODE)
#define ONE_KNOWN_DW_OP(NAME, CODE) case CODE: return #CODE;
      ALL_KNOWN_DW_OP
#undef ONE_KNOWN_DW_OP
#undef ONE_KNOWN_DW_OP_DESC
    default:
      return nullptr;
    }
}

static unsigned int
dwarf_locexpr_opcode_value (std::string str)
{
#define ONE_KNOWN_DW_OP_DESC(NAME, CODE, DESC) ONE_KNOWN_DW_OP (NAME, CODE)
#define ONE_KNOWN_DW_OP(NAME, CODE) if (str == #CODE) return CODE;
      ALL_KNOWN_DW_OP
#undef ONE_KNOWN_DW_OP
#undef ONE_KNOWN_DW_OP_DESC

  throw std::runtime_error (std::string ("Unknown locexpr opcode: ") + str);
}


/* Used by all dwarf_foo_name functions.  */
static const char *
string_or_unknown (const char *known, unsigned int code,
                   unsigned int lo_user, unsigned int hi_user,
		   bool print_unknown_num)
{
  static char unknown_buf[20];

  if (known != nullptr)
    return known;

  if (lo_user != 0 && code >= lo_user && code <= hi_user)
    {
      snprintf (unknown_buf, sizeof unknown_buf, "lo_user+%#x",
		code - lo_user);
      return unknown_buf;
    }

  if (print_unknown_num)
    {
      snprintf (unknown_buf, sizeof unknown_buf, "??? (%#x)", code);
      return unknown_buf;
    }

  return "???";
}

namespace
{
  struct unsigned_constant_dom
    : public constant_dom
  {
    bool
    sign () const override
    {
      return false;
    }
  };
}

static struct
  : public unsigned_constant_dom
{
  void
  show (uint64_t v, std::ostream &o) const override
  {
    int tag = v;
    const char *ret = dwarf_tag_string (tag);
    o << string_or_unknown (ret, tag, DW_TAG_lo_user, DW_TAG_hi_user, true);
  }
} dw_tag_dom_obj;

constant_dom const &dw_tag_dom = dw_tag_dom_obj;


static struct
  : public unsigned_constant_dom
{
  virtual void
  show (uint64_t v, std::ostream &o) const
  {
    unsigned int attr = v;
    const char *ret = dwarf_attr_string (attr);
    o << string_or_unknown (ret, attr, DW_AT_lo_user, DW_AT_hi_user, true);
  }
} dw_attr_dom_obj;

constant_dom const &dw_attr_dom = dw_attr_dom_obj;


static struct
  : public unsigned_constant_dom
{
  virtual void
  show (uint64_t v, std::ostream &o) const
  {
    unsigned int form = v;
    const char *ret = dwarf_form_string (form);
    o << string_or_unknown (ret, form, 0, 0, true);
  }
} dw_form_dom_obj;

constant_dom const &dw_form_dom = dw_form_dom_obj;


static struct
  : public unsigned_constant_dom
{
  virtual void
  show (uint64_t v, std::ostream &o) const
  {
    unsigned int lang = v;
    const char *ret = dwarf_lang_string (lang);
    o << string_or_unknown (ret, lang, DW_LANG_lo_user, DW_LANG_hi_user, false);
  }
} dw_lang_dom_obj;

constant_dom const &dw_lang_dom = dw_lang_dom_obj;


static struct
  : public unsigned_constant_dom
{
  virtual void
  show (uint64_t v, std::ostream &o) const
  {
    unsigned int code = v;
    const char *ret = dwarf_inline_string (code);
    o << string_or_unknown (ret, code, 0, 0, false);
  }
} dw_inline_dom_obj;

constant_dom const &dw_inline_dom = dw_inline_dom_obj;


static struct
  : public unsigned_constant_dom
{
  virtual void
  show (uint64_t v, std::ostream &o) const
  {
    unsigned int code = v;
    const char *ret = dwarf_encoding_string (code);
    o << string_or_unknown (ret, code, DW_ATE_lo_user, DW_ATE_hi_user,false);
  }
} dw_encoding_dom_obj;

constant_dom const &dw_encoding_dom = dw_encoding_dom_obj;


static struct
  : public unsigned_constant_dom
{
  virtual void
  show (uint64_t v, std::ostream &o) const
  {
    unsigned int code = v;
    const char *ret = dwarf_access_string (code);
    o << string_or_unknown (ret, code, 0, 0, false);
  }
} dw_access_dom_obj;

constant_dom const &dw_access_dom = dw_access_dom_obj;


static struct
  : public unsigned_constant_dom
{
  virtual void
  show (uint64_t v, std::ostream &o) const
  {
    unsigned int code = v;
    const char *ret = dwarf_visibility_string (code);
    o << string_or_unknown (ret, code, 0, 0,false);
  }
} dw_visibility_dom_obj;

constant_dom const &dw_visibility_dom = dw_visibility_dom_obj;


static struct
  : public unsigned_constant_dom
{
  virtual void
  show (uint64_t v, std::ostream &o) const
  {
    unsigned int code = v;
    const char *ret = dwarf_virtuality_string (code);
    o << string_or_unknown (ret, code, 0, 0,false);
  }
} dw_virtuality_dom_obj;

constant_dom const &dw_virtuality_dom = dw_virtuality_dom_obj;


static struct
  : public unsigned_constant_dom
{
  virtual void
  show (uint64_t v, std::ostream &o) const
  {
    unsigned int code = v;
    const char *ret = dwarf_identifier_case_string (code);
    o << string_or_unknown (ret, code, 0, 0,false);
  }
} dw_identifier_case_dom_obj;

constant_dom const &dw_identifier_case_dom = dw_identifier_case_dom_obj;


static struct
  : public unsigned_constant_dom
{
  virtual void
  show (uint64_t v, std::ostream &o) const
  {
    unsigned int code = v;
    const char *ret = dwarf_calling_convention_string (code);
    o << string_or_unknown (ret, code, DW_CC_lo_user, DW_CC_hi_user,false);
  }
} dw_calling_convention_dom_obj;

constant_dom const &dw_calling_convention_dom = dw_calling_convention_dom_obj;


static struct
  : public unsigned_constant_dom
{
  virtual void
  show (uint64_t v, std::ostream &o) const
  {
    unsigned int code = v;
    const char *ret = dwarf_ordering_string (code);
    o << string_or_unknown (ret, code, 0, 0,false);
  }
} dw_ordering_dom_obj;

constant_dom const &dw_ordering_dom = dw_ordering_dom_obj;


static struct
  : public unsigned_constant_dom
{
  virtual void
  show (uint64_t v, std::ostream &o) const
  {
    unsigned int code = v;
    const char *ret = dwarf_discr_list_string (code);
    o << string_or_unknown (ret, code, 0, 0, false);
  }
} dw_discr_list_dom_obj;

constant_dom const &dw_discr_list_dom = dw_discr_list_dom_obj;


static struct
  : public unsigned_constant_dom
{
  virtual void
  show (uint64_t v, std::ostream &o) const
  {
    unsigned int code = v;
    const char *ret = dwarf_locexpr_opcode_string (code);
    o << string_or_unknown (ret, code, DW_OP_lo_user, DW_OP_hi_user,true);
  }
} dw_locexpr_opcode_dom_obj;

constant_dom const &dw_locexpr_opcode_dom = dw_locexpr_opcode_dom_obj;

constant
constant::parse (std::string str)
{
#define STARTSWITH(PREFIX) (str.substr (0, sizeof (PREFIX) - 1) == PREFIX)
#define HANDLE(PREFIX, KEY)					\
  if (STARTSWITH (PREFIX))					\
    return constant (dwarf_##KEY##_value (str), &dw_##KEY##_dom)

  HANDLE ("DW_TAG_", tag);
  HANDLE ("DW_AT_", attr);
  HANDLE ("DW_FORM_", form);
  HANDLE ("DW_LANG_", lang);
  HANDLE ("DW_INL_", inline);
  HANDLE ("DW_ATE_", encoding);
  HANDLE ("DW_ACCESS_", access);
  HANDLE ("DW_VIS_", visibility);
  HANDLE ("DW_VIRTUALITY_", virtuality);
  HANDLE ("DW_ID_", identifier_case);
  HANDLE ("DW_CC_", calling_convention);
  HANDLE ("DW_ORD_", ordering);
  HANDLE ("DW_DSC_", discr_list);
  HANDLE ("DW_OP_", locexpr_opcode);

#undef HANDLE
#undef STARTSWITH

  throw std::runtime_error (std::string ("Unknown constant: ") + str);
}

constant
constant::parse_attr (std::string str)
{
  return parse ("DW_AT_" + str);
}

constant
constant::parse_tag (std::string str)
{
  return parse ("DW_TAG_" + str);
}
