/*
  Copyright (C) 2015 Red Hat, Inc.

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

#include "libzwergP.hh"
#include "libzwerg-dw.h"

#include "builtin-dw.hh"
#include "value-dw.hh"

zw_vocabulary const *
zw_vocabulary_dwarf (zw_error **out_err)
{
  return capture_errors ([&] () {
      static zw_vocabulary v {dwgrep_vocabulary_dw ()};
      return &v;
    }, nullptr, out_err);
}

bool
zw_value_is_dwarf (zw_value const *val)
{
  return zw_value_is <value_dwarf> (val);
}

bool
zw_value_is_cu (zw_value const *val)
{
  return zw_value_is <value_cu> (val);
}

bool
zw_value_is_die (zw_value const *val)
{
  return zw_value_is <value_die> (val);
}

namespace
{
  zw_value *
  init_dwarf (char const *filename, doneness d, size_t pos, zw_error **out_err)
  {
    return capture_errors ([&] () {
	return new zw_value {std::make_unique <value_dwarf> (filename, pos, d)};
      }, nullptr, out_err);
  }
}

zw_value *
zw_value_init_dwarf (char const *filename, size_t pos, zw_error **out_err)
{
  return init_dwarf (filename, doneness::cooked, pos, out_err);
}

zw_value *
zw_value_init_dwarf_raw (char const *filename, size_t pos, zw_error **out_err)
{
  return init_dwarf (filename, doneness::raw, pos, out_err);
}

char const *
zw_value_dwarf_name (zw_value const *val)
{
  assert (val != nullptr);

  value_dwarf const *dw = value::as <value_dwarf> (val->m_value.get ());
  assert (dw != nullptr);

  return dw->get_fn ().c_str ();
}

namespace
{
  template <class T>
  T const &
  unpack (zw_value const *val)
  {
    assert (val != nullptr);

    T const *hlv = value::as <T> (val->m_value.get ());
    assert (hlv != nullptr);

    return *hlv;
  }

  value_cu const &
  cu (zw_value const *val)
  {
    return unpack <value_cu> (val);
  }
}

Dwarf_CU *
zw_value_cu_cu (zw_value const *val)
{
  return &cu (val).get_cu ();
}

Dwarf_Off
zw_value_cu_offset (zw_value const *val)
{
  return cu (val).get_offset ();
}

namespace
{
  value_die const &
  die (zw_value const *val)
  {
    return unpack <value_die> (val);
  }
}

Dwarf_Die
zw_value_die_die (zw_value const *val)
{
  return die (val).get_die ();
}
