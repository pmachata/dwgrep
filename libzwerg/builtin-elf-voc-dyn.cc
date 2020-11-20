/*
   Copyright (C) 2020 Petr Machata
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

#include <elf.h>

#include "builtin-elf-voc-dyn.hh"
#include "builtin.hh"
#include "elfcst.hh"
#include "known-elf.h"

void
dwgrep_vocabulary_elf_dyn (vocabulary &voc)
{
#define ELF_ONE_KNOWN_DT(NAME, CODE)					\
  add_builtin_constant (voc, constant (CODE, &elf_dt_dom (machine)), #CODE);

  {
    constexpr int machine = EM_NONE;
    ELF_ALL_KNOWN_DT
  }

#define ELF_ONE_KNOWN_DT_ARCH(ARCH)		\
    {						\
      constexpr int machine = EM_##ARCH;	\
      ELF_ALL_KNOWN_DT_##ARCH			\
    }
  ELF_ALL_KNOWN_DT_ARCHES

#undef ELF_ONE_KNOWN_DT_ARCH
#undef ELF_ONE_KNOWN_DT
}
