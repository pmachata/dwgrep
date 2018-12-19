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

#include <iostream>

#include "value-elf.hh"
#include "dwpp.hh"
#include "flag_saver.hh"

Dwfl_Module *
get_sole_module (Dwfl *dwfl)
{
  struct init_state
  {
    Dwfl_Module *mod;

    init_state ()
      : mod {nullptr}
    {}

    static int each_module_cb (Dwfl_Module *mod, void **userdata,
			       char const *fn, Dwarf_Addr addr, void *arg)
    {
      auto self = static_cast <init_state *> (arg);

      // For Dwarf contexts that libzwerg currently creates, there ought to be
      // exactly one module.
      assert (self->mod == nullptr);
      self->mod = mod;
      return DWARF_CB_OK;
    }
  };

  init_state ist;
  if (dwfl_getmodules (dwfl, init_state::each_module_cb, &ist, 0) == -1)
    throw_libdwfl ();

  assert (ist.mod != nullptr);
  return ist.mod;
}

std::pair <Elf *, GElf_Addr>
get_main_elf (Dwfl *dwfl)
{
  Dwfl_Module *mod = get_sole_module (dwfl);

  GElf_Addr bias;
  Elf *elf = dwfl_module_getelf (mod, &bias);
  assert (elf != nullptr);

  return {elf, bias};
}

std::pair <Dwarf *, GElf_Addr>
get_main_dwarf (Dwfl *dwfl)
{
  Dwfl_Module *mod = get_sole_module (dwfl);

  Dwarf_Addr bias;
  Dwarf *dwarf = dwfl_module_getdwarf (mod, &bias);
  assert (dwarf != nullptr);

  return {dwarf, bias};
}

size_t
get_shdrstrndx (Elf *elf)
{
  size_t ndx;
  int err = elf_getshdrstrndx (elf, &ndx);
  if (err != 0)
    throw_libelf ();
  return ndx;
}

GElf_Ehdr
get_ehdr (Dwfl *dwfl)
{
  Elf *elf = get_main_elf (dwfl).first;
  // Note: Bias ignored.
  GElf_Ehdr ehdr;
  if (gelf_getehdr (elf, &ehdr) == nullptr)
    throw_libelf ();

  return ehdr;
}

value_type const value_elf::vtype = value_type::alloc ("T_ELF",
R"docstring(

xxx document me.

)docstring");

void
value_elf::show (std::ostream &o) const
{
  o << "<Elf \"" << m_fn << "\">";
}

cmp_result
value_elf::cmp (value const &that) const
{
  if (auto v = value::as <value_elf> (&that))
    return compare (m_dwctx->get_dwfl (), v->m_dwctx->get_dwfl ());
  else
    return cmp_result::fail;
}

std::unique_ptr <value>
value_elf::clone () const
{
  return std::make_unique <value_elf> (*this);
}

value_type const value_elf_section::vtype = value_type::alloc ("T_ELFSCN",
R"docstring(

xxx document me.

)docstring");

value_elf_section::value_elf_section (std::shared_ptr <dwfl_context> dwctx,
				      Elf_Scn *scn, size_t pos, doneness d)
  : value {vtype, pos}
  , doneness_aspect {d}
  , m_dwctx {dwctx}
  , m_scn {scn}
{}

value_elf_section::value_elf_section (std::shared_ptr <dwfl_context> dwctx,
				      size_t index, size_t pos, doneness d)
  : value {vtype, pos}
  , doneness_aspect {d}
  , m_dwctx {dwctx}
  , m_scn {elf_getscn (get_main_elf (dwctx->get_dwfl ()).first, index)}
{
  if (m_scn == nullptr)
    throw_libelf ();
}

void
value_elf_section::show (std::ostream &o) const
{
  o << "<ElfScn xxx>";
}

cmp_result
value_elf_section::cmp (value const &that) const
{
  if (auto v = value::as <value_elf_section> (&that))
    return compare (m_scn, v->m_scn);
  else
    return cmp_result::fail;
}

std::unique_ptr <value>
value_elf_section::clone () const
{
  return std::make_unique <value_elf_section> (*this);
}

value_type const value_strtab_entry::vtype
    = value_type::alloc ("T_STRTAB_ENTRY",
R"docstring(

xxx document me.

)docstring");

void
value_strtab_entry::show (std::ostream &o) const
{
  m_strval.show (o);
  ios_flag_saver s {o};
  o << "@" << std::hex << std::showbase << m_offset;
}

cmp_result
value_strtab_entry::cmp (value const &that) const
{
  if (auto v = value::as <value_strtab_entry> (&that))
    {
      cmp_result r = compare (m_offset, v->m_offset);
      if (r != cmp_result::equal)
	return r;
      return compare (m_strval.get_string (), v->m_strval.get_string ());
    }
  else
    return cmp_result::fail;
}

std::unique_ptr <value>
value_strtab_entry::clone () const
{
  return std::make_unique <value_strtab_entry> (*this);
}
