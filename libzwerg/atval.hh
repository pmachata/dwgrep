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

#ifndef _ATVAL_H_
#define _ATVAL_H_

#include "dwfl_context.hh"
#include "op.hh"

#include "value-dw.hh"
#include "value-aset.hh"

// Obtain a value of ATTR at DIE.
std::unique_ptr <value_producer <value>>
at_value_cooked (std::shared_ptr <dwfl_context> dwctx,
		 value_die const &die, Dwarf_Attribute attr);

// Obtain a raw, uninterpreted value of ATTR at DIE.
std::unique_ptr <value_producer <value>>
at_value_raw (Dwarf_Attribute attr);

// Obtain DIE's ranges.
value_aset die_ranges (Dwarf_Die die);

std::unique_ptr <value_producer <value>>
dwop_number (std::shared_ptr <dwfl_context> dwctx,
	     Dwarf_Attribute const &attr, Dwarf_Op const *op);

std::unique_ptr <value_producer <value>>
dwop_number2 (std::shared_ptr <dwfl_context> dwctx,
	      Dwarf_Attribute const &attr, Dwarf_Op const *op);

#endif /* _ATVAL_H_ */
