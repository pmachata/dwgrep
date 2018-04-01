/*
  Copyright (C) 2017, 2018 Petr Machata
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

  // zw_error instances represent errors that libzwerg propagates to
  // the user.  They are allocated by libzwerg proper, but need to be
  // freed explicitly by calling zw_error_destroy.
  //
  // Example:
  // {
  //   zw_error *err;
  //   zw_vocabulary *voc = zw_vocabulary_init (&err);
  //   if (voc == NULL)
  //     {
  //       fprintf (stderr, "libzwerg: can't initialize vocabulary: %s\n",
  //                zw_error_message (err));
  //       zw_error_destroy (err);
  //       return -1;
  //     }
  // }
  typedef struct zw_error zw_error;

  // A vocabulary is a mapping between names and corresponding
  // operations.  E.g. it might map the word "child" to an operation
  // that takes a DIE on TOS and yields all its children.
  typedef struct zw_vocabulary zw_vocabulary;

  // Objects of type zw_query are internal representations of parsed
  // Zwerg queries.
  typedef struct zw_query zw_query;

  // Constant domains, or cdoms, are used to distinguish meaning of
  // constants that happen to have the same underlying value.  The
  // domain decides whether two values compare equal even though they
  // are from different domains (e.g. values with hex domain compare
  // equal to values with oct domain, but values with DW_TAG_ domain
  // don't compare equal to values with DW_AT_ domain, even if they
  // happen to have the same underlying integral value).
  //
  // Domain objects can be obtained by calling some zw_cdom_*
  // functions, there is no direct support for supplying custom
  // domains.
  typedef struct zw_cdom zw_cdom;

  // Objects of type zw_value represent Zwerg values--be they strings,
  // integers, or something specialized, such as DWARF's.  Given a
  // value, it is possible to ask whether it belongs to some class,
  // e.g. one could call zw_value_is_const to determine whether the
  // value is a constant.
  //
  // Example:
  // {
  //   if (zw_value_is_const (&val))
  //     dump_const (os, val, fmt);
  //   else if (zw_value_is_str (&val))
  //     dump_string (os, val, fmt);
  //   else if (zw_value_is_seq (&val))
  //     ....;
  // }
  //
  // There is no externally-visible global registry of value types,
  // and this one-by-one querying is the only way to determine the
  // value type.
  typedef struct zw_value zw_value;

  // Stacks are both input and output to query execution.  They
  // consist of values.
  typedef struct zw_stack zw_stack;

  // zw_result represents a result set of query execution.  It can
  // produce individual stacks of values that the query yielded.
  typedef struct zw_result zw_result;


  // Free the resources associated with ERR.
  void zw_error_destroy (zw_error *err);

  // Get an error message describing the error ERR.
  char const *zw_error_message (zw_error const *err);


  // Create a new vocabulary.  Returns NULL on error, in which case it
  // sets *OUT_ERR.  OUT_ERR shall be non-NULL.
  zw_vocabulary *zw_vocabulary_init (zw_error **out_err);

  // Release resources associated with VOC.
  void zw_vocabulary_destroy (zw_vocabulary *voc);

  // Get core vocabulary.  Returns NULL on error, in which case it
  // sets *OUT_ERR.  OUT_ERR shall be non-NULL.
  zw_vocabulary const *zw_vocabulary_core (zw_error **out_err);

  // Get DWARF vocabulary.  Returns NULL on error, in which case it
  // sets *OUT_ERR.  OUT_ERR shall be non-NULL.
  zw_vocabulary const *zw_vocabulary_dwarf (zw_error **out_err);

  // Get ELF vocabulary.  Returns NULL on error, in which case it
  // sets *OUT_ERR.  OUT_ERR shall be non-NULL.
  zw_vocabulary const *zw_vocabulary_elf (zw_error **out_err);

  // Add contents of vocabulary TO_ADD to vocabulary VOC.  Returns
  // false on error, in which case it sets *OUT_ERR.  OUT_ERR shall be
  // non-NULL.
  //
  // Example:
  // {
  //   zw_vocabulary *voc = zw_vocabulary_init (&err);
  //   // handle error
  //
  //   zw_vocabulary *core_voc = zw_vocabulary_core (&err);
  //   // handle error
  //   if (! zw_vocabulary_add (voc, core_voc, &err))
  //     // handle error
  //
  //   zw_vocabulary *dw_voc = zw_vocabulary_dwarf (&err);
  //   // handle error
  //   if (! zw_vocabulary_add (voc, dw_voc, &err))
  //     // handle error
  //
  //   // At this point, VOC can be used to handle typical dwgrep
  //   // queries.
  // }
  bool zw_vocabulary_add (zw_vocabulary *voc, zw_vocabulary const *to_add,
			  zw_error **out_err);


  // Create and return a new stack.  Returns NULL on error, in which
  // case it sets *OUT_ERR.  OUT_ERR shall be non-NULL.
  zw_stack *zw_stack_init (zw_error **out_err);

  // Release resources associated with STACK, including any values
  // that it contains.
  void zw_stack_destroy (zw_stack *stack);

  // Add a new value to STACK.  The value added is actually a clone of
  // VALUE, not VALUE itself.  Returns false on error, in which case
  // it sets *OUT_ERR.  OUT_ERR shall be non-NULL.
  bool zw_stack_push (zw_stack *stack, zw_value const *value,
		      zw_error **out_err);

  // Like zw_stack_push, but VALUE itself is taken instead of a clone.
  // In that case, VALUE shall have been allocated using one of the
  // libzwerg API interfaces.  Returns false on error, in which case
  // it sets *OUT_ERR, and *still* takes VALUE. OUT_ERR shall be
  // non-NULL.
  bool zw_stack_push_take (zw_stack *stack, zw_value *value,
			   zw_error **out_err);

  // Return number of values that STACK holds.
  size_t zw_stack_depth (zw_stack const *stack);

  // Return a (non-NULL) pointer to value on STACK in given DEPTH. TOS has
  // depth 0. DEPTH shall be smaller than stack depth.
  zw_value const *zw_stack_at (zw_stack const *stack, size_t depth);


  // Parse a QUERY, given vocabulary VOC.  Returns a zw_query object
  // that represents the parsed query.  Returns NULL on error, in
  // which case it sets *OUT_ERR.  OUT_ERR shall be non-NULL.
  zw_query *zw_query_parse (zw_vocabulary const *voc, char const *query,
			    zw_error **out_err);

  // Like zw_query_parse, but string does not have to be
  // NUL-terminated, but has a given length QUERY_LEN instead.
  zw_query *zw_query_parse_len (zw_vocabulary const *voc,
				char const *query, size_t query_len,
				zw_error **out_err);

  // Release resources associated with QUERY.
  void zw_query_destroy (zw_query *query);

  // Run a QUERY on a given INPUT STACK.  Returns a result set from
  // which individual resulting stacks can be pulled.  Returns NULL on
  // error, in which case it sets *OUT_ERR.  OUT_ERR shall be
  // non-NULL.
  zw_result *zw_query_execute (zw_query const *query,
			       zw_stack const *input_stack,
			       zw_error **out_err);

  // Pull next output stack from RESULT.  Returns true and sets
  // *OUT_STACK to the stack with output values, or to NULL, if there
  // are no more results.  Returns false on error, in which case it
  // sets *OUT_ERR.  OUT_ERR shall be non-NULL.
  bool zw_result_next (zw_result *result,
		       zw_stack **out_stack, zw_error **out_err);

  // Release resources associated with RESULT.
  void zw_result_destroy (zw_result *result);


  /**
   * Values.
   */

  // Get position of value.  Each Zwerg operator numbers elements that
  // it produces, and stores number of each element along with the
  // element.  That number can be recalled by zw_value_pos.
  size_t zw_value_pos (zw_value const *val);

  // Release any resources associated with VAL.
  void zw_value_destroy (zw_value *val);

  // Returns a copy of VALUE, with a new position POS. Returns NULL on
  // error, in which case it sets *OUT_ERR. OUT_ERR shall be non-NULL.
  zw_value *zw_value_clone (zw_value const *val, size_t pos,
			    zw_error **out_err);


  /**
   * Constants.
   */

  // Create a value that represents a signed integral constant, with a
  // domain DOM.  Position of that value is POS.  Returns NULL on
  // error, in which case it sets *OUT_ERR.  OUT_ERR shall be
  // non-NULL.
  zw_value *zw_value_init_const_i64 (int64_t i, zw_cdom const *dom,
				     size_t pos, zw_error **out_err);

  // Create a value that represents an unsigned integral constant,
  // with a domain DOM.  Position of that value is POS. Returns NULL
  // on error, in which case it sets *OUT_ERR.  OUT_ERR shall be
  // non-NULL.
  zw_value *zw_value_init_const_u64 (uint64_t i, zw_cdom const *dom,
				     size_t pos, zw_error **out_err);

  // Return whether VAL is an integral constant value.
  bool zw_value_is_const (zw_value const *val);

  // Return whether VAL, which shall be integral constant value, is
  // signed value.
  bool zw_value_const_is_signed (zw_value const *cst);

  // Return unsigned value representing CST, which shall be integral
  // constant value.
  uint64_t zw_value_const_u64 (zw_value const *cst);

  // Return signed value representing CST, which shall be integral
  // constant value.
  int64_t zw_value_const_i64 (zw_value const *cst);

  // Return a string value representing how CST is formatted according
  // to its domain.  The actual underlying string can be obtained by
  // zw_value_str_str.  The returned value must eventually be
  // destroyed by the caller.  Returns NULL on error, in which case it
  // sets *OUT_ERR.  OUT_ERR shall be non-NULL.
  zw_value *zw_value_const_format (zw_value const *cst,
				   zw_error **out_err);

  // Similar to zw_value_const_format, but formats CST as a brief
  // constant (e.g., array_type instead of DW_TAG_array_type, or 1ab
  // instead of 0x1ab).
  zw_value *zw_value_const_format_brief (zw_value const *cst,
					 zw_error **out_err);

  // Return a domain associated with CST, which shall be integral
  // constant value.
  zw_cdom const *zw_value_const_dom (zw_value const *cst);


  /**
   * Constant domains.
   */

  // Return name of constant domain.
  char const *zw_cdom_name (zw_cdom const *cdom);

  // Returns whether CDOM is an arithmetic domain, meaning the
  // underlying value is meaningful in and of itself, regardless of
  // domain.
  bool zw_cdom_is_arith (zw_cdom const *cdom);

  // Return a domain representing decimal numbers.
  zw_cdom const *zw_cdom_dec (void);

  // Return a domain representing hexadecimal numbers.
  zw_cdom const *zw_cdom_hex (void);

  // Return a domain representing octal numbers.
  zw_cdom const *zw_cdom_oct (void);

  // Return a domain representing binary numbers.
  zw_cdom const *zw_cdom_bin (void);

  // Return a domain representing boolean values.
  zw_cdom const *zw_cdom_bool (void);

  /**
   * Strings.
   */

  // Create a value representing a string STR (which is
  // NUL-terminated), whose position is POS.  Returns NULL on error,
  // in which case it sets *OUT_ERR.  OUT_ERR shall be non-NULL.
  zw_value *zw_value_init_str (char const *str, size_t pos,
			       zw_error **out_err);

  // Like zw_value_init_str, but STR is not considered NUL-terminated,
  // but instead has a given length LEN.
  zw_value *zw_value_init_str_len (char const *str, size_t len, size_t pos,
				   zw_error **out_err);

  // Return whether VAL represents a string value.
  bool zw_value_is_str (zw_value const *val);

  // Return underlying string buffer of STR, which shall be a string
  // value.  Sets *LENP to a length of the string.
  char const *zw_value_str_str (zw_value const *str, size_t *lenp);


  /**
   * Sequences.
   */

  // Return whether VAL is a sequence value.
  bool zw_value_is_seq (zw_value const *val);

  // Return length of SEQ (i.e. number of held values), which shall be
  // a sequence value.
  size_t zw_value_seq_length (zw_value const *seq);

  // Return pointer to value stored at position IDX of SEQ, which
  // shall be a sequence value.  IDX shall be smaller than length of
  // the sequence.
  zw_value const *zw_value_seq_at (zw_value const *seq, size_t idx);


#ifdef __cplusplus
}
#endif

#endif
