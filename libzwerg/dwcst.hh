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

#ifndef _DWCST_H_
#define _DWCST_H_

struct constant_dom;

constant_dom const &dw_tag_dom ();
constant_dom const &dw_attr_dom ();
constant_dom const &dw_form_dom ();

constant_dom const &dw_access_dom ();
constant_dom const &dw_address_class_dom ();
constant_dom const &dw_calling_convention_dom ();
constant_dom const &dw_decimal_sign_dom ();
constant_dom const &dw_discr_list_dom ();
constant_dom const &dw_encoding_dom ();
constant_dom const &dw_endianity_dom ();
constant_dom const &dw_identifier_case_dom ();
constant_dom const &dw_inline_dom ();
constant_dom const &dw_lang_dom ();
constant_dom const &dw_locexpr_opcode_dom ();
constant_dom const &dw_macinfo_dom ();
constant_dom const &dw_macro_dom ();
constant_dom const &dw_ordering_dom ();
constant_dom const &dw_virtuality_dom ();
constant_dom const &dw_visibility_dom ();

constant_dom const &dw_address_dom ();	// Dwarf_Addr
constant_dom const &dw_offset_dom ();	// Dwarf_Off
constant_dom const &dw_abbrevcode_dom ();

#endif /* _DWCST_H_ */
