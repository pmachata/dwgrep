#ifndef _DWPP_H_
#define _DWPP_H_

#include <cassert>

inline void
throw_libdw (int dwerr = 0)
{
  if (dwerr == 0)
    dwerr = dwarf_errno ();
  assert (dwerr != 0);
  throw std::runtime_error (dwarf_errmsg (dwerr));
}

inline void
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

#endif /* _DWPP_H_ */
