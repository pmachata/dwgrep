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

#include <elf.h>
#include <iostream>
#include <map>
#include <vector>

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


// Since there are aliases in ELF_ALL_KNOWN_EM (and we do want to keep them,
// because it's still desirable to be able to use that name in a program), we
// can't use the trick with switch. At least EM_ALPHA also has a massive value,
// which precludes using a simple array storage for all names.
static void
elf_em_string_initialize (std::vector <char const *> &linear,
			  std::map <unsigned, char const *> &assoc)
{
  std::map <unsigned, char const *> all;
#define ELF_ONE_KNOWN_EM(NAME, CODE) all[CODE] = #CODE;
  ELF_ALL_KNOWN_EM;
#undef ELF_ONE_KNOWN_EM

  // Find the longest sequence that's advantageous to store linearly. On g++,
  // map node has an overhead of four words, and unordered_map seems to fare in
  // the same ballpark, plus the space for the key gives five words. So one
  // useful entry for five wasted is still a fair deal, memory-wise (and very
  // likely runtime-wise as well, since we do base add and dereference vs.
  // pointer chasing in a map).

  unsigned last = 0;   // The last item that's advantageous to keep in linear.
  unsigned useful = 0; // Usefully-spent items in linear.
  for (auto it = all.begin (); it != all.end (); ++it)
    if ((it->first + 1) / 6 <= useful++)
      last = it->first;

  for (auto it = all.begin ();
       it->first <= last && it != all.end ();
       it = all.erase (it))
    {
      linear.resize (it->first, nullptr);
      linear.push_back (it->second);
    }

  assoc = std::move (all);
}

static const char *
__elf_em_string (unsigned em, brevity brv)
{
  static std::vector <const char *> linear;
  static std::map <unsigned, const char *> assoc;
  static bool initialized;
  if (! initialized)
    {
      elf_em_string_initialize (linear, assoc);
      initialized = true;
    }

  const char *ret = nullptr;
  if (em < linear.size ())
    ret = abbreviate (linear[em], sizeof "EM", brv);
  else
    {
      auto it = assoc.find (em);
      if (it != assoc.end ())
	ret = abbreviate (it->second, sizeof "EM", brv);
    }

  return ret;
}

static const char *
elf_em_string (int em, brevity brv)
{
  if (em < 0)
    return nullptr;
  else
    return __elf_em_string (static_cast <unsigned> (em), brv);
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

zw_cdom const &
elf_em_dom ()
{
  static dw_simple_dom dom {"EM", elf_em_string, 0, 0, false};
  return dom;
}
