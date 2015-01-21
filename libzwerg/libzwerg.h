/*
  Copyright (C) 2014, 2015 Red Hat, Inc.

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

  typedef struct zw_error zw_error;
  typedef struct zw_vocabulary zw_vocabulary;
  typedef struct zw_query zw_query;
  typedef struct zw_cdom zw_cdom;
  typedef struct zw_value zw_value;
  typedef struct zw_stack zw_stack;
  typedef struct zw_result zw_result;


  void zw_error_destroy (zw_error *err);

  char const *zw_error_message (zw_error const *err);


  zw_vocabulary *zw_vocabulary_init (zw_error **out_err);
  void zw_vocabulary_destroy (zw_vocabulary *voc);

  zw_vocabulary const *zw_vocabulary_core (zw_error **out_err);
  zw_vocabulary const *zw_vocabulary_dwarf (zw_error **out_err);

  bool zw_vocabulary_add (zw_vocabulary *voc, zw_vocabulary const *to_add,
			  zw_error **out_err);

  bool zw_vocabulary_add_argv (zw_vocabulary *voc,
			       zw_value const *values[], size_t nvalues,
			       zw_error **out_err);


  zw_stack *zw_stack_init (zw_error **out_err);

  void zw_stack_destroy (zw_stack *stack);

  bool zw_stack_push (zw_stack *stack, zw_value const *value,
		      zw_error **out_err);
  bool zw_stack_push_take (zw_stack *stack, zw_value *value,
			   zw_error **out_err);

  size_t zw_stack_depth (zw_stack const *stack);

  zw_value const *zw_stack_at (zw_stack const *stack, size_t depth);

  bool zw_stack_dump_xxx (zw_stack const *stack, zw_error **out_err);


  zw_query *zw_query_parse (zw_vocabulary const *voc, char const *query,
			    zw_error **out_err);

  zw_query *zw_query_parse_len (zw_vocabulary const *voc,
				char const *query, size_t query_len,
				zw_error **out_err);

  void zw_query_destroy (zw_query *query);


  zw_result *zw_query_execute (zw_query const *query,
			       zw_stack const *input_stack,
			       zw_error **out_err);

  bool zw_result_next (zw_result *result,
		       zw_stack **out_stack, zw_error **out_err);

  void zw_result_destroy (zw_result *result);


  /**
   * Values.
   */

  size_t zw_value_pos (zw_value const *val);

  zw_value *zw_value_clone (zw_value const *val);

  // N.B.: Run NAME as a program on an empty stack, collect the
  // singular pushed value, and return.
  zw_value *zw_value_init_named (zw_vocabulary const *voc, char const *name,
				 zw_error **out_err);

  char *zw_value_show (zw_value const *val, zw_error **out_err);

  void zw_value_destroy (zw_value *val);


  /**
   * Constants.
   */

  zw_value *zw_value_init_const_i64 (int64_t i, zw_cdom const *dom,
				     size_t pos, zw_error **out_err);

  zw_value *zw_value_init_const_u64 (uint64_t i, zw_cdom const *dom,
				     size_t pos, zw_error **out_err);

  bool zw_value_is_const (zw_value const *val);

  bool zw_value_const_is_signed (zw_value const *cst);

  uint64_t zw_value_const_u64 (zw_value const *cst);

  int64_t zw_value_const_i64 (zw_value const *cst);

  zw_value *zw_value_const_format (zw_value const *cst,
				   zw_error **out_err);

  zw_cdom const *zw_value_const_dom (zw_value const *cst);


  /**
   * Constant domains.
   */

  char const *zw_cdom_name (zw_cdom const *cdom);

  bool zw_cdom_is_arith (zw_cdom const *cdom);

  zw_cdom const *zw_cdom_dec (void);
  zw_cdom const *zw_cdom_hex (void);
  zw_cdom const *zw_cdom_oct (void);
  zw_cdom const *zw_cdom_bin (void);
  zw_cdom const *zw_cdom_bool (void);

  /**
   * Strings.
   */

  zw_value *zw_value_init_str (char const *str, size_t pos,
			       zw_error **out_err);

  zw_value *zw_value_init_str_len (char const *str, size_t len, size_t pos,
				   zw_error **out_err);

  bool zw_value_is_str (zw_value const *str);

  char const *zw_value_str_str (zw_value const *str, size_t *lenp);


  /**
   * Sequences.
   */

  zw_value *zw_value_init_seq (zw_value *vals[], size_t nvals,
			       size_t pos, zw_error **out_err);

  bool zw_value_is_seq (zw_value const *val);

  bool zw_value_seq_append (zw_value *seq, zw_value *val, zw_error **out_err);

  size_t zw_value_seq_length (zw_value const *seq);

  zw_value const *zw_value_seq_at (zw_value const *seq, size_t idx);


#ifdef __cplusplus
}
#endif

#endif
