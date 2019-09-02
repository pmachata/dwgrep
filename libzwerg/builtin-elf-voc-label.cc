/*
   Copyright (C) 2018, 2019 Petr Machata
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

#include "builtin-elf-voc-label.hh"
#include "builtin-elfscn.hh"
#include "builtin-elfrel.hh"
#include "builtin-symbol.hh"
#include "elfcst.hh"
#include "known-elf.h"

struct constant_descriptor
{
  int value;
  char const *name;
  char const *qname;
  char const *bname;
};

template <class Pred>
void
register_constants (vocabulary &voc, zw_cdom const *dom, int machine,
		    constant_descriptor cds[], size_t ncds)
{
  for (size_t i = 0; i < ncds; ++i)
    {
      constant_descriptor &cd = cds[i];
      add_builtin_constant (voc, constant (cd.value, dom), cd.name);

      auto t = std::make_shared <overload_tab> ();

      t->add_pred_overload <Pred> (cd.value, machine);

      voc.add (std::make_shared <overloaded_pred_builtin> (cd.qname, t, true));
      voc.add (std::make_shared <overloaded_pred_builtin> (cd.bname, t, false));
    }
}

void
dwgrep_vocabulary_elf_label (vocabulary &voc)
{
  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_label_symbol> ();
    t->add_op_overload <op_label_elfscn> ();
    t->add_op_overload <op_label_elfrel> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("label", t));
  }

#define ELF_ONE_KNOWN_STT(NAME, CODE)					\
  add_builtin_constant (voc, constant (CODE, &elfsym_stt_dom (machine)), #CODE);

  {
    constexpr int machine = EM_NONE;
    ELF_ALL_KNOWN_STT
  }

#define ELF_ONE_KNOWN_STT_ARCH(ARCH)		\
    {						\
      constexpr int machine = EM_##ARCH;	\
      ELF_ALL_KNOWN_STT_##ARCH			\
    }
  ELF_ALL_KNOWN_STT_ARCHES

#undef ELF_ONE_KNOWN_STT_ARCH
#undef ELF_ONE_KNOWN_STT

#define ELF_ONE_KNOWN_SHT(NAME, CODE)					\
	{CODE, #CODE, "?" #CODE, "!" #CODE},

#define __ELF_ONE_KNOWN_SHT_ARCH(ARCH, ALL_KNOWN)				\
  {									\
    constexpr int machine = EM_##ARCH;					\
    zw_cdom const &dom = elf_sht_dom (machine);				\
    constant_descriptor cds[] =						\
      {									\
	ALL_KNOWN							\
      };								\
    register_constants <pred_label_elfscn> (voc, &dom, machine, cds,	\
					    sizeof cds / sizeof cds[0]); \
  }

  __ELF_ONE_KNOWN_SHT_ARCH (NONE, ELF_ALL_KNOWN_SHT)

#define ELF_ONE_KNOWN_SHT_ARCH(ARCH)					\
  __ELF_ONE_KNOWN_SHT_ARCH (ARCH, ELF_ALL_KNOWN_SHT_##ARCH)

  ELF_ALL_KNOWN_SHT_ARCHES

#undef ELF_ONE_KNOWN_SHT_ARCH
#undef ELF_ONE_KNOWN_SHT

#define ELF_ONE_KNOWN_R(NAME, CODE)					\
	{CODE, #CODE, "?" #CODE, "!" #CODE},

#define ELF_ONE_KNOWN_R_ARCH(ARCH)					\
    {									\
      constexpr int machine = EM_##ARCH;				\
      zw_cdom const &dom = elf_r_dom (machine);				\
      constant_descriptor cds[] =					\
	{								\
	  ELF_ALL_KNOWN_R_##ARCH					\
	};								\
      register_constants <pred_label_elfrel> (voc, &dom, machine, cds,	\
					      sizeof cds / sizeof cds[0]); \
    }

  ELF_ALL_KNOWN_R_ARCHES

#undef ELF_ONE_KNOWN_R_ARCH
#undef ELF_ONE_KNOWN_R
}
