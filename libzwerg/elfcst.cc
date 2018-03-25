/*
   Copyright (C) 2018 Petr Machata

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

#include <iostream>
#include <elf.h>

#include "constant.hh"
#include "dwcst.hh"
#include "known-elf.h"

static const char *
elf_class_string (int cls, brevity brv)
{
  switch (cls)
    {
    case ELFCLASSNONE:
      return abbreviate ("ELFCLASSNONE", 8, brv);
    case ELFCLASS32:
      return abbreviate ("ELFCLASS32", 8, brv);
    case ELFCLASS64:
      return abbreviate ("ELFCLASS64", 8, brv);
    default:
      return nullptr;
    }
}

static const char *
elf_et_string (int et, brevity brv)
{
  switch (et)
    {
#define ELF_ONE_KNOWN_ET(NAME, CODE)			\
      case CODE: return abbreviate (#CODE, sizeof "ET", brv);
      ELF_ALL_KNOWN_ET
#undef ELF_ONE_KNOWN_ET
    default:
      return nullptr;
    }
}

zw_cdom const &
elf_class_dom ()
{
  static dw_simple_dom dom {"ELFCLASS", elf_class_string, 0, 0, false};
  return dom;
}

zw_cdom const &
elf_et_dom ()
{
  struct dom
    : public constant_dom
  {
    void
    show (mpz_class const &v, std::ostream &o, brevity brv) const override
    {
      unsigned code;
      if (auto maybe_code = uint_from_mpz (v))
	code = *maybe_code;
      else
	{
	  o << "<invalid code>";
	  return;
	}

      char unknown_buf[40];
      if (char const *known = elf_et_string (code, brv))
	o << known;
      else
	{
	  if (! format_user_range (brv, code, ET_LOOS, ET_HIOS,
				   "EM_", "LOOS",
				   unknown_buf, sizeof unknown_buf)
	      && ! format_user_range (brv, code, ET_LOPROC, ET_HIPROC,
				      "EM_", "LOPROC",
				      unknown_buf, sizeof unknown_buf))
	    format_unknown (brv, code, "EM_",
			    unknown_buf, sizeof unknown_buf);
	  o << unknown_buf;
	}
    }

    char const *
    name () const override
    {
      return "ET_";
    }

    char const *
    docstring () const override
    {
      return dw_simple_dom::generic_docstring ();
    }
  };

  static dom dom;
  return dom;
}
