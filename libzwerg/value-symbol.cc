/*
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

#include <iostream>
#include <climits>
#include "value-symbol.hh"
#include "std-memory.hh"
#include "known-elf.h"
#include "dwcst.hh"
#include "elfcst.hh"

value_type const value_symbol::vtype = value_type::alloc ("T_ELFSYM",
R"docstring(

Values of this type represent ELF symbol table entries::

	$ dwgrep/dwgrep dwgrep/dwgrep -e symbol
	[...]
	89:	0x0000000060cd90      0 NOTYPE	LOCAL	DEFAULT	__init_array_start
	90:	0x0000000060cdb8      0 OBJECT	LOCAL	DEFAULT	_DYNAMIC
	91:	0x00000000405f20   1001 FUNC	GLOBAL	DEFAULT	_ZN6dumper10dump_charpERSoPKcmNS_6formatE
	92:	0x0000000060d3e0      0 NOTYPE	WEAK	DEFAULT	data_start
	93:	0x0000000060d400    280 OBJECT	GLOBAL	DEFAULT	_ZSt3cin@@GLIBCXX_3.4
	94:	0000000000000000      0 FUNC	GLOBAL	DEFAULT	_ZNSs6appendEPKcm@@GLIBCXX_3.4
	[...]

)docstring");

void
value_symbol::show (std::ostream &o) const
{
  o << get_type () << ' ' << get_name () << '@' << get_address ();
}

std::unique_ptr <value>
value_symbol::clone () const
{
  return std::make_unique <value_symbol> (*this);
}

cmp_result
value_symbol::cmp (value const &that) const
{
  if (auto v = value::as <value_symbol> (&that))
    return compare (m_symidx, v->m_symidx);
  else
    return cmp_result::fail;
}

constant
value_symbol::get_type () const
{
  return constant {GELF_ST_TYPE (get_symbol ().st_info),
		   &elfsym_stt_dom (get_dwctx ()->get_machine ())};
}

constant
value_symbol::get_address () const
{
  return constant {get_symbol ().st_value, &dw_address_dom ()};
}

namespace
{
  void
  show (char const *pfx, char const *str, std::ostream &o, brevity brv)
  {
    if (brv == brevity::full)
      o << pfx << '_';
    o << str;
  }

#define ELF_ONE_KNOWN_STT(SHORT, LONG)		\
  case LONG:					\
    return show ("STT", #SHORT, o, brv);

  struct elfsym_stt_dom_t
    : public elf_cst_dom
  {
    void
    show (mpz_class const &v, std::ostream &o, brevity brv) const override
    {
      using ::show;
      switch (int code = v < 0 || v.uval () > INT_MAX ? -1 : v.uval ())
	{
	  ELF_ALL_KNOWN_STT
	default:
	  show_unknown ("STT", code,
			STT_LOOS, STT_HIOS, STT_LOPROC, STT_HIPROC, o, brv);
	}
    }

    char const *
    name () const override
    {
      return "STT_";
    }
  };

#define ELF_ONE_KNOWN_STT_ARCH(ARCH)					\
  struct elfsym_stt_##ARCH##_dom_t					\
    : public elfsym_stt_dom_t						\
  {									\
    void								\
    show (mpz_class const &v, std::ostream &o, brevity brv) const override \
    {									\
      using ::show;							\
      switch (v < 0 || v.uval () > INT_MAX ? -1 : v.uval ())		\
	{								\
	  ELF_ALL_KNOWN_STT_##ARCH					\
	}								\
      elfsym_stt_dom_t::show (v, o, brv);				\
    }									\
									\
    virtual constant_dom const *					\
    most_enclosing (mpz_class const &v) const override			\
    {									\
      if (v < STT_LOOS)							\
	return &elfsym_stt_dom (EM_NONE);				\
      return this;							\
    }									\
  };

  ELF_ALL_KNOWN_STT_ARCHES

#undef ELF_ONE_KNOWN_STT_ARCH
#undef ELF_ONE_KNOWN_STT


#define ELF_ONE_KNOWN_STB(SHORT, LONG)		\
  case LONG:					\
    return show ("STB", #SHORT, o, brv);

  struct elfsym_stb_dom_t
    : public elf_cst_dom
  {
    void
    show (mpz_class const &v, std::ostream &o, brevity brv) const override
    {
      using ::show;
      switch (int code = v < 0 || v.uval () > INT_MAX ? -1 : v.uval ())
	{
	  ELF_ALL_KNOWN_STB
	default:
	  show_unknown ("STB", code,
			STB_LOOS, STB_HIOS, STB_LOPROC, STB_HIPROC, o, brv);
	}
    }

    char const *
    name () const override
    {
      return "STB_";
    }
  };

#define ELF_ONE_KNOWN_STB_ARCH(ARCH)					\
  struct elfsym_stb_##ARCH##_dom_t					\
    : public elfsym_stb_dom_t						\
  {									\
    void								\
    show (mpz_class const &v, std::ostream &o, brevity brv) const override \
    {									\
      using ::show;							\
      switch (v < 0 || v.uval () > INT_MAX ? -1 : v.uval ())		\
	{								\
	  ELF_ALL_KNOWN_STB_##ARCH					\
	}								\
      elfsym_stb_dom_t::show (v, o, brv);				\
    }									\
									\
    virtual constant_dom const *					\
    most_enclosing (mpz_class const &v) const override			\
    {									\
      if (v < STB_LOOS)							\
	return &elfsym_stb_dom (EM_NONE);				\
      return this;							\
    }									\
  };

  ELF_ALL_KNOWN_STB_ARCHES

#undef ELF_ONE_KNOWN_STB_ARCH
#undef ELF_ONE_KNOWN_STB

  struct elfsym_stv_dom_t
    : public elf_cst_dom
  {
    void
    show (mpz_class const &v, std::ostream &o, brevity brv) const override
    {
      using ::show;
      switch (int code = v < 0 || v.uval () > INT_MAX ? -1 : v.uval ())
	{
	case STV_DEFAULT:	return show ("STV", "DEFAULT", o, brv);
	case STV_INTERNAL:	return show ("STV", "INTERNAL", o, brv);
	case STV_HIDDEN:	return show ("STV", "HIDDEN", o, brv);
	case STV_PROTECTED:	return show ("STV", "PROTECTED", o, brv);
	default:
	  show_unknown ("STV", code, 0, -1, 0, -1, o, brv);
	}
    }

    char const *
    name () const override
    {
      return "STV_";
    }
  };
}

constant_dom const &
elfsym_stt_dom (int machine)
{
  switch (machine)
    {
#define ELF_ONE_KNOWN_STT_ARCH(ARCH)		\
      case EM_##ARCH:				\
	{					\
	  static elfsym_stt_##ARCH##_dom_t dom;	\
	  return dom;				\
	}
      ELF_ALL_KNOWN_STT_ARCHES
#undef ELF_ONE_KNOWN_STT_ARCH
    }

  static elfsym_stt_dom_t dom;
  return dom;
}

constant_dom const &
elfsym_stb_dom (int machine)
{
  switch (machine)
    {
#define ELF_ONE_KNOWN_STB_ARCH(ARCH)		\
      case EM_##ARCH:				\
	{					\
	  static elfsym_stb_##ARCH##_dom_t dom;	\
	  return dom;				\
	}
      ELF_ALL_KNOWN_STB_ARCHES
#undef ELF_ONE_KNOWN_STB_ARCH
    }

  static elfsym_stb_dom_t dom;
  return dom;
}

constant_dom const &
elfsym_stv_dom ()
{
  static elfsym_stv_dom_t dom;
  return dom;
}
