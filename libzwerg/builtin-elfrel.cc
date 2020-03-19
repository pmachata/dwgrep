/*
   Copyright (C) 2019, 2020 Petr Machata
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
#include "dwcst.hh"
#include "dwpp.hh"
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

std::unique_ptr <value_symbol>
op_symbol_elfrel::operate (std::unique_ptr <value_elf_rel> a) const
{
  auto symndx = GELF_R_SYM (a->get_rela ().r_info);
  if (symndx == STN_UNDEF)
    return nullptr;

  Elf *elf = get_main_elf (a->get_dwctx ()->get_dwfl ()).first;
  if (elf == nullptr)
    throw_libelf ();

  // Symbol table.
  Elf_Scn *symtabscn = elf_getscn (elf, a->get_symtabndx ());
  if (symtabscn == nullptr)
    throw_libelf ();

  Elf_Data *symtabdata = get_data (symtabscn); // non-null or throws

  // String table associated with that symbol table.
  size_t symstrtabndx = get_link (symtabscn);

  // Finally fetch the symbol.
  GElf_Sym symbol;
  if (gelf_getsym (symtabdata, symndx, &symbol) == nullptr)
    throw_libelf ();

  const char *name = elf_strptr (elf, symstrtabndx, symbol.st_name);
  if (name == nullptr)
    throw_libelf ();

  return std::make_unique <value_symbol> (a->get_dwctx (), symbol, name,
					  symndx, 0, a->get_doneness ());
}

std::string
op_symbol_elfrel::docstring ()
{
  return
R"docstring(

xxx

)docstring";
}

value_cst
op_offset_elfrel::operate (std::unique_ptr <value_elf_rel> a) const
{
  return value_cst {constant {a->get_rela ().r_offset, &dw_offset_dom ()}, 0};
}

std::string
op_offset_elfrel::docstring ()
{
  return
R"docstring(

xxx

)docstring";
}
