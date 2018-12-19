/*
   Copyright (C) 2018 Petr Machata
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

#include "builtin-elf-voc-flags.hh"
#include "builtin-elf.hh"
#include "builtin-elfscn.hh"
#include "elfcst.hh"
#include "known-elf.h"

void
dwgrep_vocabulary_elf_flags (vocabulary &voc)
{
  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_eflags_elf <value_elf>> ();
    t->add_op_overload <op_eflags_elf <value_dwarf>> ();
    t->add_op_overload <op_flags_elfscn> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("flags", t));
  }

  // N.B.: There are no ?EF_ and !EF_ words. Flags need to be tested explicitly,
  // because the field often includes various bitfields and it's not a priori
  // clear how the flag should be tested.
#define ELF_ONE_KNOWN_EF(NAME, CODE)					\
  add_builtin_constant (voc, constant (CODE, &elf_ef_dom (machine)), #CODE);

#define ELF_ONE_KNOWN_EF_ARCH(ARCH)		\
    {						\
      constexpr int machine = EM_##ARCH;	\
      ELF_ALL_KNOWN_EF_##ARCH			\
    }
  ELF_ALL_KNOWN_EF_ARCHES

#undef ELF_ONE_KNOWN_EF_ARCH
#undef ELF_ONE_KNOWN_EF

#define CONSTANT(CODE, NAME, DOM, PRED)					\
    {									\
      add_builtin_constant (voc, constant (CODE, DOM), NAME);		\
									\
      auto t = std::make_shared <overload_tab> ();			\
									\
      t->add_pred_overload <PRED> (CODE);				\
									\
      voc.add (std::make_shared <overloaded_pred_builtin>		\
	       ("?" NAME, t, true));					\
      voc.add (std::make_shared <overloaded_pred_builtin>		\
	       ("!" NAME, t, false));					\
    }

#define ELF_ONE_KNOWN_SHF(NAME, CODE)					\
  CONSTANT(CODE, #CODE, &elf_shf_dom (machine), pred_flags_elfscn);

    {
      constexpr int machine = EM_NONE;
      ELF_ALL_KNOWN_SHF
    }

#define ELF_ONE_KNOWN_SHF_ARCH(ARCH)		\
    {						\
      constexpr int machine = EM_##ARCH;	\
      ELF_ALL_KNOWN_SHF_##ARCH			\
    }
  ELF_ALL_KNOWN_SHF_ARCHES

#undef ELF_ONE_KNOWN_SHF_ARCH
#undef ELF_ONE_KNOWN_SHF
}
