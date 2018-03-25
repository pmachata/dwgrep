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

#include "../extern/optional.hpp"

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

struct dw_simple_dom
  : public constant_dom
{
  char const *m_name;
  char const *(*m_stringer) (int, brevity);
  int m_low_user;
  int m_high_user;
  bool m_print_unknown;

  dw_simple_dom (char const *name, char const *(*stringer) (int, brevity),
		 int low_user, int high_user, bool print_unknown)
    : m_name {name}
    , m_stringer {stringer}
    , m_low_user {low_user}
    , m_high_user {high_user}
    , m_print_unknown {print_unknown}
  {}

  void show (mpz_class const &v, std::ostream &o, brevity brv) const override;
  char const *name () const override;
  char const *docstring () const override;

  static char const *generic_docstring ();
};

nonstd::optional <unsigned> uint_from_mpz (mpz_class const &v);

bool
format_user_range (brevity brv,
		   unsigned int code,
                   unsigned int lo, unsigned int hi,
		   const char *prefix, const char *base,
		   char *buf, size_t buf_size);

void
format_unknown (brevity brv,
		unsigned int code,
		const char *prefix,
		char *buf, size_t buf_size);

#endif /* _DWCST_H_ */
