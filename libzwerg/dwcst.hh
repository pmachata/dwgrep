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

#ifndef _DWCST_H_
#define _DWCST_H_

#include "constant.hh"

zw_cdom const &dw_tag_dom ();
zw_cdom const &dw_attr_dom ();
zw_cdom const &dw_form_dom ();

zw_cdom const &dw_access_dom ();
zw_cdom const &dw_address_class_dom ();
zw_cdom const &dw_calling_convention_dom ();
zw_cdom const &dw_decimal_sign_dom ();
zw_cdom const &dw_discr_list_dom ();
zw_cdom const &dw_encoding_dom ();
zw_cdom const &dw_endianity_dom ();
zw_cdom const &dw_defaulted_dom ();
zw_cdom const &dw_identifier_case_dom ();
zw_cdom const &dw_inline_dom ();
zw_cdom const &dw_lang_dom ();
zw_cdom const &dw_locexpr_opcode_dom ();
zw_cdom const &dw_macinfo_dom ();
zw_cdom const &dw_macro_dom ();
zw_cdom const &dw_ordering_dom ();
zw_cdom const &dw_virtuality_dom ();
zw_cdom const &dw_visibility_dom ();

zw_cdom const &dw_address_dom ();	// Dwarf_Addr
zw_cdom const &dw_offset_dom ();	// Dwarf_Off
zw_cdom const &dw_abbrevcode_dom ();

#endif /* _DWCST_H_ */
