/*
   Copyright (C) 2014 Red Hat, Inc.
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

#ifndef _DWPP_H_
#define _DWPP_H_

#include <cassert>
#include <elfutils/libdwfl.h>
#include <elfutils/libdw.h>

inline __attribute__ ((noreturn)) void
throw_libdwfl (int dwerr = 0)
{
  if (dwerr == 0)
    dwerr = dwfl_errno ();
  assert (dwerr != 0);
  throw std::runtime_error (dwfl_errmsg (dwerr));
}

inline __attribute__ ((noreturn)) void
throw_libdw (int dwerr = 0)
{
  if (dwerr == 0)
    dwerr = dwarf_errno ();
  assert (dwerr != 0);
  throw std::runtime_error (dwarf_errmsg (dwerr));
}

inline __attribute__ ((noreturn)) void
throw_libelf ()
{
  int elferr = elf_errno ();
  assert (elferr != 0);
  throw std::runtime_error (elf_errmsg (elferr));
}

inline bool
dwpp_siblingof (Dwarf_Die *die, Dwarf_Die *result)
{
  switch (dwarf_siblingof (die, result))
    {
    case -1:
      throw_libdw ();
    case 0:
      return true;
    case 1:
      return false;
    default:
      abort ();
    }
}

inline void
dwpp_child (Dwarf_Die *die, Dwarf_Die *result)
{
  if (dwarf_child (die, result) != 0)
    throw_libdw ();
}

inline Dwarf_Die
dwpp_offdie (Dwarf *dbg, Dwarf_Off offset)
{
  Dwarf_Die result;
  if (dwarf_offdie (dbg, offset, &result) == nullptr)
    throw_libdw ();
  return result;
}

inline bool
dwpp_attr_integrate (Dwarf_Die *die, unsigned int search_name,
		     Dwarf_Attribute *result)
{
  // Clear the error code.
  dwarf_errno ();
  if (dwarf_attr_integrate (die, search_name, result) == nullptr)
    {
      int dwerr = dwarf_errno ();
      if (dwerr != 0)
	throw_libdw (dwerr);
      else
	return false;
    }
  return true;
}

inline Dwarf_Off
dwpp_abbrev_offset (Dwarf_Abbrev &abbrev)
{
  // Evil, evil hack.  But the easiest way to pry offset off ABBREV's
  // hands as well, libdw won't help us in this regard.
  return reinterpret_cast <Dwarf_Off &> (abbrev);
}

inline size_t
dwpp_abbrev_attrcnt (Dwarf_Abbrev &abbrev)
{
  size_t ret;
  if (dwarf_getattrcnt (&abbrev, &ret) != 0)
    throw_libdw ();
  return ret;
}

#endif /* _DWPP_H_ */
