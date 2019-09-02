/*
   Copyright (C) 2019 Petr Machata
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

#include "builtin-elfrel.hh"
#include "elfcst.hh"

value_cst
op_label_elfrel::operate (std::unique_ptr <value_elf_rel> a) const
{
  auto type = GELF_R_TYPE (a->get_rela ().r_info);
  return value_cst {constant {type, &elf_r_dom (a->get_machine ())}, 0};
}

std::string
op_label_elfrel::docstring ()
{
  return
R"docstring(

xxx

)docstring";
}

pred_result
pred_label_elfrel::result (value_elf_rel &a) const
{
  auto type = GELF_R_TYPE (a.get_rela ().r_info);
  return pred_result (m_machine == a.get_machine ()
		      && m_type == type);
}

std::string
pred_label_elfrel::docstring ()
{
  return
R"docstring(

xxx

)docstring";
}
