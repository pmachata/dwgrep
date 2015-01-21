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

extern "C" zw_vocabulary const *
zw_vocabulary_dwarf (zw_error **out_err)
{
  return capture_errors ([&] () {
      static zw_vocabulary v {dwgrep_vocabulary_dw ()};
      return &v;
    }, nullptr, out_err);
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
