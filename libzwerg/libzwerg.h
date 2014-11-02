/*
  Copyright (C) 2014 Red Hat, Inc.

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

#ifndef LIBZWERG_H_
#define LIBZWERG_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

  typedef struct zw_error_s *zw_error;

  typedef struct zw_vocabulary_s *zw_vocabulary;
  typedef struct zw_vocabulary_s const *c_zw_vocabulary;

  typedef struct zw_query_s *zw_query;

  typedef struct zw_constant_dom const *c_zw_constant_dom;

  typedef struct zw_value_s *zw_value;


  void zw_error_destroy (zw_error err);

  char const *zw_error_message (zw_error err);


  zw_vocabulary zw_vocabulary_init (zw_error out_err);
  void zw_vocabulary_destroy (zw_vocabulary voc);

  c_zw_vocabulary zw_vocabulary_core (void);
  c_zw_vocabulary zw_vocabulary_dwarf (void);

  bool zw_vocabulary_add (zw_vocabulary voc, c_zw_vocabulary to_add,
			  zw_error out_err);

  bool zw_vocabulary_add_argv (zw_vocabulary voc,
			       zw_value values[], size_t nvalues,
			       zw_error out_err);


  zw_query zw_query_parse (c_zw_vocabulary voc, char const *query,
			   zw_value stack[], size_t stksz,
			   zw_error out_err);

  void zw_query_destroy (zw_query query);


  c_zw_constant_dom zw_constant_dom_dec (void);
  c_zw_constant_dom zw_constant_dom_hex (void);
  c_zw_constant_dom zw_constant_dom_oct (void);
  c_zw_constant_dom zw_constant_dom_bin (void);
  c_zw_constant_dom zw_constant_dom_bool (void);
  c_zw_constant_dom zw_constant_dom_column (void);
  c_zw_constant_dom zw_constant_dom_line (void);

  c_zw_constant_dom zw_constant_dom_address (void);
  c_zw_constant_dom zw_constant_dom_offset (void);
  c_zw_constant_dom zw_constant_dom_abbrevcode (void);

  c_zw_constant_dom zw_constant_dom_tag (void);
  c_zw_constant_dom zw_constant_dom_attr (void);
  c_zw_constant_dom zw_constant_dom_form (void);
  c_zw_constant_dom zw_constant_dom_lang (void);
  c_zw_constant_dom zw_constant_dom_macinfo (void);
  c_zw_constant_dom zw_constant_dom_macro (void);
  c_zw_constant_dom zw_constant_dom_inline (void);
  c_zw_constant_dom zw_constant_dom_encoding (void);
  c_zw_constant_dom zw_constant_dom_access (void);
  c_zw_constant_dom zw_constant_dom_visibility (void);
  c_zw_constant_dom zw_constant_dom_virtuality (void);
  c_zw_constant_dom zw_constant_dom_identifier_case (void);
  c_zw_constant_dom zw_constant_dom_calling_convention (void);
  c_zw_constant_dom zw_constant_dom_ordering (void);
  c_zw_constant_dom zw_constant_dom_discr_list (void);
  c_zw_constant_dom zw_constant_dom_decimal_sign (void);
  c_zw_constant_dom zw_constant_dom_locexpr_opcode (void);
  c_zw_constant_dom zw_constant_dom_address_class (void);
  c_zw_constant_dom zw_constant_dom_endianity (void);


  zw_value zw_value_init_const_i64 (int64_t i, c_zw_constant_dom dom,
					  zw_error out_err);

  zw_value zw_value_init_const_u64 (uint64_t i, c_zw_constant_dom dom,
					  zw_error out_err);

  zw_value zw_value_init_str (char const *str, zw_error out_err);

  zw_value zw_value_init_str_len (char const *str, size_t len,
					zw_error out_err);

  // N.B.: Run NAME as a program on an empty stack collect the
  // singular pushed value, and return.
  zw_value zw_value_init_named (c_zw_vocabulary voc, char const *name,
				zw_error out_err);

  zw_value zw_value_init_dwarf (char const *filename);
  zw_value zw_value_init_dwarf_raw (char const *filename);

#ifdef __cplusplus
}
#endif

#endif
