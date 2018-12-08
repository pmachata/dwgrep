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

#ifndef BUILTIN_ELF_VOC_H
#define BUILTIN_ELF_VOC_H

#include <memory>

#define ADD_ELF_CONSTANT(CODE, NAME, DOM, PRED)				\
    {									\
      add_builtin_constant (voc, constant (CODE, DOM), NAME);		\
									\
      auto t = std::make_shared <overload_tab> ();			\
									\
      t->add_pred_overload <PRED <value_dwarf>> (CODE);			\
      t->add_pred_overload <PRED <value_elf>> (CODE);			\
									\
      voc.add (std::make_shared <overloaded_pred_builtin>		\
	       ("?" NAME, t, true));					\
      voc.add (std::make_shared <overloaded_pred_builtin>		\
	       ("!" NAME, t, false));					\
    }

struct vocabulary;
std::unique_ptr <vocabulary> dwgrep_vocabulary_elf ();

#endif /* BUILTIN_ELF_VOC_H */
