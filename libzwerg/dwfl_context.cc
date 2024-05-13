/*
   Copyright (C) 2014, 2015 Red Hat, Inc.
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

#include "dwfl_context.hh"
#include "cache.hh"
#include "dwit.hh"

struct dwfl_context::pimpl
{
  parent_cache m_parcache;
  root_cache m_rootcache;

  Dwarf_Off
  find_parent (Dwarf_Die die)
  {
    return m_parcache.find (die);
  }

  bool
  is_root (Dwarf_Die die)
  {
    return m_rootcache.is_root (die);
  }
};

dwfl_context::dwfl_context (std::shared_ptr <Dwfl> dwfl)
  : m_pimpl {std::make_unique <pimpl> ()}
  , m_dwfl {dwfl}
{}

dwfl_context::~dwfl_context ()
{}

Dwarf_Off
dwfl_context::find_parent (Dwarf_Die die)
{
  return m_pimpl->find_parent (die);
}

bool
dwfl_context::is_root (Dwarf_Die die)
{
  return m_pimpl->is_root (die);
}

int
dwfl_context::get_machine () const
{
  int machine = EM_NONE;
  GElf_Addr bias;
  for (auto it = dwfl_module_iterator {m_dwfl.get ()};
       it != dwfl_module_iterator::end (); ++it)
    if (Elf *elf = dwfl_module_getelf (*it, &bias))
      {
	GElf_Ehdr ehdr;
	if (gelf_getehdr (elf, &ehdr) == nullptr)
	  throw_libelf ();
	assert (machine == EM_NONE || machine == ehdr.e_machine);
	machine = ehdr.e_machine;
      }
    else
      throw_libdwfl ();

  // This better be true.  That symbol must have come from somewhere.
  assert (machine != EM_NONE);
  return machine;
}
