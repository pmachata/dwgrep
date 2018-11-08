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
