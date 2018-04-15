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


class linear_map
{
  std::vector <const char *> m_linear;
  std::map <unsigned, const char *> m_assoc;

public:
  linear_map (std::map <unsigned, const char *> all)
  {
    // Find the longest sequence that's advantageous to store linearly. On g++,
    // map node has an overhead of four words, and unordered_map seems to fare
    // in the same ballpark, plus the space for the key gives five words. So one
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
	m_linear.resize (it->first, nullptr);
	m_linear.push_back (it->second);
      }

    m_assoc = std::move (all);
  }

  char const *find (int key)
  {
    if (key < 0)
      return nullptr;
    return find (static_cast <unsigned> (key));
  }

  char const *find (unsigned key)
  {
    if (key < m_linear.size ())
      return m_linear[key];

    auto it = m_assoc.find (key);
    if (it != m_assoc.end ())
      return it->second;

    return nullptr;
  }
};


// Since there are aliases in ELF_ALL_KNOWN_EM (and we do want to keep them,
// because it's still desirable to be able to use that name in a program), we
// can't use the trick with switch. At least EM_ALPHA also has a massive value,
// which precludes using a simple array storage for all names.
struct elf_em_strings
  : public linear_map
{
  std::map <unsigned, char const *>
  all () const
  {
    std::map <unsigned, char const *> ret;
#define ELF_ONE_KNOWN_EM(NAME, CODE) ret[CODE] = #CODE;
    ELF_ALL_KNOWN_EM
#undef ELF_ONE_KNOWN_EM
    return ret;
  }

  elf_em_strings ()
    : linear_map {all ()}
  {}
};

static const char *
elf_em_string (int em, brevity brv)
{
  static elf_em_strings ss;
  return abbreviate (ss.find (em), sizeof "EM", brv);
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


// Elf flags shouldn't have any common constant block. Make sure it's so.
#define ELF_ONE_KNOWN_EF(SHORT, LONG)		\
  static_assert (false, "Unexpected CPU-unspecific EF_ constant.");
ELF_ALL_KNOWN_EF
#undef ELF_ONE_KNOWN_EF

// A dictionary of words for each machine. Since there are aliases, we need to
// use the linear map.
#define ELF_ONE_KNOWN_EF(NAME, CODE)		\
  ret[CODE] = #CODE;

#define ELF_ONE_KNOWN_EF_ARCH(ARCH)				\
  struct elf_ef_##ARCH##_strings				\
    : public linear_map						\
  {								\
    std::map <unsigned, char const *>				\
    all () const						\
    {								\
      std::map <unsigned, char const *> ret;			\
      ELF_ALL_KNOWN_EF_##ARCH					\
      return ret;						\
    }								\
								\
    elf_ef_##ARCH##_strings ()					\
      : linear_map {all ()}					\
    {}								\
  };								\
								\
  static const char *						\
  elf_ef_##ARCH##_string (unsigned em, brevity brv)		\
  {								\
    static elf_ef_##ARCH##_strings ss;				\
    return abbreviate (ss.find (em), sizeof "EF", brv);		\
  }

ELF_ALL_KNOWN_EF_ARCHES

#undef ELF_ONE_KNOWN_EF_ARCH
#undef ELF_ONE_KNOWN_EF

// Domain for each EF arch.
#define ELF_ONE_KNOWN_EF_ARCH(ARCH)					\
  struct elfsym_ef_##ARCH##_dom_t					\
    : public constant_dom						\
  {									\
    void								\
    show (mpz_class const &v, std::ostream &o, brevity brv) const override \
    {									\
      if (auto maybe_code = uint_from_mpz (v))				\
	{								\
	  if (auto s = elf_ef_##ARCH##_string (*maybe_code, brv))	\
	    o << s;							\
	  else								\
	    {								\
	      char unknown_buf[40];					\
	      format_unknown (brv, *maybe_code, "EF_",			\
			      unknown_buf, sizeof unknown_buf);		\
	      o << unknown_buf;						\
	    }								\
	}								\
      else								\
	o << "<invalid code>";						\
    }									\
									\
    char const *							\
    name () const override						\
    {									\
      return "EF_" #ARCH;						\
    }									\
									\
    bool								\
    safe_arith () const override					\
    {									\
      return true;							\
    }									\
  };

ELF_ALL_KNOWN_EF_ARCHES

#undef ELF_ONE_KNOWN_EF_ARCH

static char const *
fallback_dom_stringer (int ef, brevity brv)
{
  // EF domain doesn't have any generic block at all.
  return nullptr;
}

zw_cdom const &
elf_ef_dom (int machine)
{
  switch (machine)
    {
#define ELF_ONE_KNOWN_EF_ARCH(ARCH)		\
      case EM_##ARCH:				\
	{					\
	  static elfsym_ef_##ARCH##_dom_t dom;	\
	  return dom;				\
	}
      ELF_ALL_KNOWN_EF_ARCHES
#undef ELF_ONE_KNOWN_EF_ARCH
    }

  static dw_simple_dom dom {"EF", fallback_dom_stringer, 0, 0, true};
  return dom;
}
