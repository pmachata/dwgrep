/*
   Copyright (C) 2017, 2018 Petr Machata
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

#ifndef ALL_KNOWN_DW_MACRO_GNU
# define ALL_KNOWN_DW_MACRO_GNU \
  ONE_KNOWN_DW_MACRO_GNU (define, DW_MACRO_GNU_define) \
  ONE_KNOWN_DW_MACRO_GNU (define_indirect, DW_MACRO_GNU_define_indirect) \
  ONE_KNOWN_DW_MACRO_GNU (end_file, DW_MACRO_GNU_end_file) \
  ONE_KNOWN_DW_MACRO_GNU (start_file, DW_MACRO_GNU_start_file) \
  ONE_KNOWN_DW_MACRO_GNU (transparent_include, DW_MACRO_GNU_transparent_include) \
  ONE_KNOWN_DW_MACRO_GNU (undef, DW_MACRO_GNU_undef) \
  ONE_KNOWN_DW_MACRO_GNU (undef_indirect, DW_MACRO_GNU_undef_indirect) \
  /* End of DW_MACRO_GNU_*.  */
#endif
