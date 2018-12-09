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

#include "builtin-elf-voc-elf.hh"
#include "builtin-elf.hh"

void
dwgrep_vocabulary_elf_elf (vocabulary &voc)
{
  auto t = std::make_shared <overload_tab> ();

  t->add_op_overload <op_elf_dwarf> ();

  voc.add (std::make_shared <overloaded_op_builtin> ("elf", t));
}