/*
   Copyright (C) 2009, 2010, 2011, 2012, 2014, 2015 Red Hat, Inc.
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

#include "dwit.hh"

namespace
{
  std::pair <Dwarf *, Dwarf_Addr>
  getdwarf (Dwfl_Module *module)
  {
    assert (module != nullptr);
    Dwarf_Addr addr;
    Dwarf *ret = dwfl_module_getdwarf (module, &addr);
    if (ret == nullptr)
      throw_libdwfl ();
    return std::make_pair (ret, addr);
  }
}

Dwarf *
dwfl_module::dwarf ()
{
  return getdwarf (m_module).first;
}

Dwarf_Addr
dwfl_module::bias ()
{
  return getdwarf (m_module).second;
}

int
dwfl_module_iterator::module_cb (Dwfl_Module *mod, void **data,
				 const char *name,
				 Dwarf_Addr addr, void *arg)
{
  auto self = static_cast <dwfl_module_iterator *> (arg);
  self->m_module = dwfl_module {mod};
  return DWARF_CB_ABORT;
}

void
dwfl_module_iterator::move ()
{
  m_offset = dwfl_getmodules (m_dwfl, module_cb, this, m_offset);
  if (m_offset == -1)
    throw_libdwfl ();
}

dwfl_module_iterator::dwfl_module_iterator (ptrdiff_t off)
  : m_dwfl {nullptr}
  , m_offset {off}
{}

dwfl_module_iterator::dwfl_module_iterator (Dwfl *dwfl)
  : m_dwfl {dwfl}
  , m_offset {0}
{
  move ();
}

dwfl_module_iterator
dwfl_module_iterator::end ()
{
  return dwfl_module_iterator ((ptrdiff_t) 0);
}

dwfl_module_iterator &
dwfl_module_iterator::operator++ ()
{
  assert (*this != end ());
  move ();
  return *this;
}

dwfl_module_iterator
dwfl_module_iterator::operator++ (int)
{
  auto ret = *this;
  ++*this;
  return ret;
}

dwfl_module
dwfl_module_iterator::operator* () const
{
  return m_module;
}

bool
dwfl_module_iterator::operator== (dwfl_module_iterator const &that) const
{
  assert (m_dwfl == nullptr || that.m_dwfl == nullptr
	  || m_dwfl == that.m_dwfl);
  return m_offset == that.m_offset;
}

bool
dwfl_module_iterator::operator!= (dwfl_module_iterator const &that) const
{
  return !(*this == that);
}


cu_iterator::cu_iterator (Dwarf_Off off)
  : m_dw (nullptr)
  , m_offset (off)
  , m_old_offset (0)
  , m_cudie ({})
{}

void
cu_iterator::move ()
{
  assert (*this != end ());
  do
    {
      m_old_offset = m_offset;
      size_t hsize;
      if (dwarf_nextcu (m_dw, m_offset, &m_offset, &hsize,
			nullptr, nullptr, nullptr) != 0)
	done ();
      else if (dwarf_offdie (m_dw, m_old_offset + hsize, &m_cudie) == nullptr)
	continue;
    }
  while (false);
}

void
cu_iterator::done ()
{
  *this = end ();
}

cu_iterator::cu_iterator (Dwarf *dw)
  : m_dw {dw}
  , m_offset {0}
  , m_old_offset {0}
  , m_cudie {}
{
  move ();
}

cu_iterator::cu_iterator (Dwarf *dw, Dwarf_Die cudie)
  : m_dw {dw}
  , m_offset {dwarf_dieoffset (&cudie) - dwarf_cuoffset (&cudie)}
  , m_old_offset {0}
  , m_cudie {}
{
  move ();
}

cu_iterator
cu_iterator::end ()
{
  return cu_iterator ((Dwarf_Off)-1);
}

bool
cu_iterator::operator== (cu_iterator const &other) const
{
  return m_offset == other.m_offset;
}

bool
cu_iterator::operator!= (cu_iterator const &other) const
{
  return !(*this == other);
}

cu_iterator
cu_iterator::operator++ ()
{
  move ();
  return *this;
}

cu_iterator
cu_iterator::operator++ (int)
{
  cu_iterator tmp = *this;
  ++*this;
  return tmp;
}

Dwarf_Off
cu_iterator::offset () const
{
  return m_old_offset;
}

Dwarf_Die *
cu_iterator::operator* ()
{
  return &m_cudie;
}


child_iterator::child_iterator ()
  : m_die {(void *) -1}
{}

child_iterator::child_iterator (Dwarf_Die parent)
{
  if (! dwpp_child (parent, m_die))
    *this = end ();
}

child_iterator
child_iterator::end ()
{
  return {};
}

bool
child_iterator::operator== (child_iterator const &other) const
{
  return m_die.addr == other.m_die.addr;
}

bool
child_iterator::operator!= (child_iterator const &other) const
{
  return ! (*this == other);
}

child_iterator
child_iterator::operator++ ()
{
  assert (*this != end ());
  if (! dwpp_siblingof (m_die, m_die))
    *this = end ();
  return *this;
}

child_iterator
child_iterator::operator++ (int)
{
  child_iterator ret = *this;
  ++*this;
  return ret;
}

Dwarf_Die *
child_iterator::operator* ()
{
  assert (*this != end ());
  return &m_die;
}


all_dies_iterator::all_dies_iterator (Dwarf_Off offset)
  : m_cuit (cu_iterator::end ())
{}

all_dies_iterator::all_dies_iterator (Dwarf *dw)
  : all_dies_iterator (cu_iterator {dw})
{}

all_dies_iterator::all_dies_iterator (cu_iterator const &cuit)
  : m_cuit (cuit)
  , m_die (**m_cuit)
{
  m_stack = std::make_shared<std::vector<Dwarf_Off>>();
}

all_dies_iterator
all_dies_iterator::end ()
{
  return all_dies_iterator ((Dwarf_Off)-1);
}

bool
all_dies_iterator::operator== (all_dies_iterator const &other) const
{
  return m_cuit == other.m_cuit
    && *m_stack == *other.m_stack
    && (m_cuit == cu_iterator::end ()
	|| m_die.addr == other.m_die.addr);
}

bool
all_dies_iterator::operator!= (all_dies_iterator const &other) const
{
  return !(*this == other);
}

all_dies_iterator
all_dies_iterator::operator++ ()
{
  Dwarf_Die child;
  if (dwpp_child (m_die, child))
    {
      m_stack->push_back (dwarf_dieoffset (&m_die));
      m_die = child;
      return *this;
    }

  do
    switch (dwarf_siblingof (&m_die, &m_die))
      {
      case -1:
	throw_libdw ();
      case 0:
	return *this;
      case 1:
	// No sibling found.  Go a level up and retry, unless this
	// was a sole, childless CU DIE.
	if (! m_stack->empty ())
	  {
	    if (dwarf_offdie (m_cuit.m_dw, m_stack->back (), &m_die) == nullptr)
	      throw_libdw ();
	    m_stack->pop_back ();
	  }
      }
  while (!m_stack->empty ());

  m_die = **++m_cuit;
  return *this;
}

all_dies_iterator
all_dies_iterator::operator++ (int)
{
  all_dies_iterator prev = *this;
  ++*this;
  return prev;
}

Dwarf_Die *
all_dies_iterator::operator* ()
{
  return &m_die;
}

std::vector<Dwarf_Die>
all_dies_iterator::stack () const
{
  std::vector<Dwarf_Die> ret;
  for (all_dies_iterator it = *this; it != end (); it = it.parent ())
    ret.push_back (**it);
  std::reverse (ret.begin (), ret.end ());
  return ret;
}

all_dies_iterator
all_dies_iterator::parent () const
{
  assert (*this != end ());
  if (m_stack->empty ())
    return end ();

  all_dies_iterator ret = *this;
  if (dwarf_offdie (m_cuit.m_dw, m_stack->back (), &ret.m_die) == nullptr)
    throw_libdw ();
  ret.m_stack->pop_back ();
  return ret;
}

cu_iterator
all_dies_iterator::cu () const
{
  return m_cuit;
}
