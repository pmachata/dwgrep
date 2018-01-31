/*
   Copyright (C) 2018 Petr Machata
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

#ifndef _DWFL_CONTEXT_H_
#define _DWFL_CONTEXT_H_

#include <memory>
#include <elfutils/libdwfl.h>

// This represents a Dwfl handle together with some query caches.
class dwfl_context
{
  class pimpl;
  std::unique_ptr <pimpl> m_pimpl;
  std::shared_ptr <Dwfl> m_dwfl;

public:
  explicit dwfl_context (std::shared_ptr <Dwfl> dwfl);
  ~dwfl_context ();

  Dwfl *get_dwfl ()
  { return &*m_dwfl; }

  Dwarf_Off find_parent (Dwarf_Die die);
  bool is_root (Dwarf_Die die);
  int get_machine () const;
};

std::shared_ptr <Dwfl> open_dwfl (std::string const &fn,
				  const Dwfl_Callbacks &callbacks);

#endif /* _DWFL_CONTEXT_H_ */
