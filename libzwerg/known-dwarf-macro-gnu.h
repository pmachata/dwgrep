/*
   Copyright (C) 2017 Petr Machata
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

#ifndef DWARF_ALL_KNOWN_DW_MACRO_GNU
# define DWARF_ALL_KNOWN_DW_MACRO_GNU \
  DWARF_ONE_KNOWN_DW_MACRO_GNU (define, DW_MACRO_GNU_define) \
  DWARF_ONE_KNOWN_DW_MACRO_GNU (define_indirect, DW_MACRO_GNU_define_indirect) \
  DWARF_ONE_KNOWN_DW_MACRO_GNU (end_file, DW_MACRO_GNU_end_file) \
  DWARF_ONE_KNOWN_DW_MACRO_GNU (start_file, DW_MACRO_GNU_start_file) \
  DWARF_ONE_KNOWN_DW_MACRO_GNU (transparent_include, DW_MACRO_GNU_transparent_include) \
  DWARF_ONE_KNOWN_DW_MACRO_GNU (undef, DW_MACRO_GNU_undef) \
  DWARF_ONE_KNOWN_DW_MACRO_GNU (undef_indirect, DW_MACRO_GNU_undef_indirect) \
  /* End of DW_MACRO_GNU_*.  */
#endif
