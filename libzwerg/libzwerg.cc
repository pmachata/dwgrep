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

#include "libzwergP.hh"

#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

#include "builtin.hh"
#include "init.hh"
#include "op.hh"
#include "parser.hh"
#include "stack.hh"
#include "tree.hh"

#include "value-cst.hh"
#include "value-str.hh"
#include "value-seq.hh"

extern "C" void
zw_error_destroy (zw_error *err)
{
  delete err;
}

extern "C" char const *
zw_error_message (zw_error const *err)
{
  return err->m_message.c_str ();
}

extern "C" zw_vocabulary *
zw_vocabulary_init (zw_error **out_err)
{
  return capture_errors ([&] () {
      return new zw_vocabulary { std::make_unique <vocabulary> () };
    }, nullptr, out_err);
}

extern "C" void
zw_vocabulary_destroy (zw_vocabulary *voc)
{
  delete voc;
}

extern "C" zw_vocabulary const *
zw_vocabulary_core (zw_error **out_err)
{
  return capture_errors ([&] () {
      static zw_vocabulary v {dwgrep_vocabulary_core ()};
      return &v;
    }, nullptr, out_err);
}

extern "C" bool
zw_vocabulary_add (zw_vocabulary *voc, zw_vocabulary const *to_add,
		   zw_error **out_err)
{
  assert (voc != nullptr);
  assert (to_add != nullptr);
  assert (voc->m_voc != nullptr);
  assert (to_add->m_voc != nullptr);
  return capture_errors ([&] () {
      voc->m_voc = std::make_unique <vocabulary> (*voc->m_voc, *to_add->m_voc);
      return true;
    }, false, out_err);
}


zw_stack *
zw_stack_init (zw_error **out_err)
{
  return capture_errors ([&] () {
      return new zw_stack;
    }, nullptr, out_err);
}

void
zw_stack_destroy (zw_stack *stack)
{
  delete stack;
}

bool
zw_stack_push (zw_stack *stack, zw_value const *value, zw_error **out_err)
{
  return capture_errors ([&] () {
      stack->m_values.push_back
	(std::make_unique <zw_value> (value->m_value->clone ()));
      return true;
    }, false, out_err);
}

bool
zw_stack_push_take (zw_stack *stack, zw_value *value, zw_error **out_err)
{
  return capture_errors ([&] () {
      stack->m_values.push_back
	(std::make_unique <zw_value> (std::move (value->m_value)));
      zw_value_destroy (value);
      return true;
    }, false, out_err);
}

size_t
zw_stack_depth (zw_stack const *stack)
{
  return stack->m_values.size ();
}

zw_value const *
zw_stack_at (zw_stack const *stack, size_t depth)
{
  assert (stack != nullptr);
  assert (depth < stack->m_values.size ());
  return stack->m_values[stack->m_values.size () - 1 - depth].get ();
}


zw_query *
zw_query_parse (zw_vocabulary const *voc, char const *query,
		zw_error **out_err)
{
  return zw_query_parse_len (voc, query, strlen (query), out_err);
}

zw_query *
zw_query_parse_len (zw_vocabulary const *voc,
		    char const *query, size_t query_len,
		    zw_error **out_err)
{
  return capture_errors ([&] () {
      tree t = parse_query (*voc->m_voc, {query, query_len});
      t.simplify ();
      return new zw_query { t };
    }, nullptr, out_err);
}

void
zw_query_destroy (zw_query *query)
{
  delete query;
}

void
zw_value_destroy (zw_value *value)
{
  delete value;
}


zw_result *
zw_query_execute (zw_query const *query, zw_stack const *input_stack,
		  zw_error **out_err)
{
  return capture_errors ([&] () {
      auto stk = std::make_unique <stack> ();
      for (auto const &emt: input_stack->m_values)
	stk->push (emt->m_value->clone ());
      auto upstream = std::make_shared <op_origin> (std::move (stk));
      return new zw_result { query->m_query.build_exec (upstream) };
    }, nullptr, out_err);
}

bool
zw_result_next (zw_result *result, zw_stack **out_stack, zw_error **out_err)
{
  return capture_errors ([&] () {
      std::unique_ptr <stack> ret = result->m_op->next ();
      if (ret == nullptr)
	{
	  *out_stack = nullptr;
	  return true;
	}

      size_t sz = ret->size ();

      std::vector <std::unique_ptr <zw_value>> values;
      values.resize (sz);

      auto it = values.rbegin ();
      for (size_t i = 0; i < sz; ++i)
	*it++ = std::make_unique <zw_value> (ret->pop ());

      *out_stack = new zw_stack { std::move (values) };
      return true;
    }, false, out_err);
}

void
zw_result_destroy (zw_result *result)
{
  delete result;
}

bool
zw_value_is_const (zw_value const *val)
{
  return zw_value_is <value_cst> (val);
}

bool
zw_value_is_str (zw_value const *val)
{
  return zw_value_is <value_str> (val);
}

bool
zw_value_is_seq (zw_value const *val)
{
  return zw_value_is <value_seq> (val);
}

namespace
{
  constant
  extract_constant (zw_value const *val)
  {
    assert (val != nullptr);
    value_cst const *cst = value::as <value_cst> (val->m_value.get ());
    assert (cst != nullptr);
    return cst->get_constant ();
  }
}

bool
zw_value_const_is_signed (zw_value const *val)
{
  return extract_constant (val).value ().m_sign == signedness::sign;
}

uint64_t
zw_value_const_u64 (zw_value const *val)
{
  auto v = extract_constant (val).value ();
  assert (v.m_sign == signedness::unsign);
  return v.uval ();
}

int64_t
zw_value_const_i64 (zw_value const *val)
{
  auto v = extract_constant (val).value ();
  assert (v.m_sign == signedness::sign);
  return v.sval ();
}

zw_value *
zw_value_const_format (zw_value const *val, zw_error **out_err)
{
  std::stringstream ss;
  ss << extract_constant (val);
  std::string const &s = ss.str ();
  return zw_value_init_str_len (s.c_str (), s.size (), 0, out_err);
}

char const *
zw_cdom_name (zw_cdom const *cdom)
{
  assert (cdom != nullptr);
  return cdom->m_cdom.name ();
}

bool
zw_cdom_is_arith (zw_cdom const *cdom)
{
  assert (cdom != nullptr);
  return cdom->m_cdom.safe_arith ();
}

zw_cdom const *
zw_cdom_dec (void)
{
  static zw_cdom cdom {dec_constant_dom};
  return &cdom;
}

zw_cdom const *
zw_cdom_hex (void)
{
  static zw_cdom cdom {hex_constant_dom};
  return &cdom;
}

zw_cdom const *
zw_cdom_oct (void)
{
  static zw_cdom cdom {oct_constant_dom};
  return &cdom;
}

zw_cdom const *
zw_cdom_bin (void)
{
  static zw_cdom cdom {bin_constant_dom};
  return &cdom;
}

zw_cdom const *
zw_cdom_bool (void)
{
  static zw_cdom cdom {bool_constant_dom};
  return &cdom;
}

namespace
{
  template <class T>
  zw_value *
  init_const (T i, zw_cdom const *dom, size_t pos, zw_error **out_err)
  {
    return capture_errors ([&] () {
	constant cst {i, &dom->m_cdom};
	return new zw_value {std::make_unique <value_cst> (cst, pos)};
      }, nullptr, out_err);
  }
}

zw_value *
zw_value_init_const_i64 (int64_t i, zw_cdom const *dom,
			 size_t pos, zw_error **out_err)
{
  return init_const (i, dom, pos, out_err);
}

zw_value *
zw_value_init_const_u64 (uint64_t i, zw_cdom const *dom,
			 size_t pos, zw_error **out_err)
{
  return init_const (i, dom, pos, out_err);
}

zw_value *
zw_value_init_str (char const *str, size_t pos,
		   zw_error **out_err)
{
  return zw_value_init_str_len (str, strlen (str), pos, out_err);
}

zw_value *
zw_value_init_str_len (char const *cstr, size_t len, size_t pos,
		       zw_error **out_err)
{
  return capture_errors ([&] () {
      std::string str {cstr, len};
      return new zw_value {std::make_unique <value_str> (str, pos)};
    }, nullptr, out_err);
}

char const *
zw_value_str_str (zw_value const *val, size_t *lenp)
{
  assert (val != nullptr);
  assert (lenp != nullptr);

  value_str const *str = value::as <value_str> (val->m_value.get ());
  assert (str != nullptr);
  *lenp = str->get_string ().length ();
  return str->get_string ().c_str ();
}

size_t
zw_value_seq_length (zw_value const *val)
{
  assert (val != nullptr);

  value_seq const *seq = value::as <value_seq> (val->m_value.get ());
  assert (seq != nullptr);
  return seq->get_seq ()->size ();
}

zw_value const *
zw_value_seq_at (zw_value const *val, size_t idx)
{
  assert (val != nullptr);

  value_seq const *seq = value::as <value_seq> (val->m_value.get ());
  assert (seq != nullptr);

  size_t sz = seq->get_seq ()->size ();
  assert (idx < sz);
  if (val->m_seq == nullptr)
    val->m_seq = std::make_unique
			<std::vector <std::unique_ptr <zw_value>>> (sz);

  auto &ref = (*val->m_seq)[idx];
  if (ref == nullptr)
    ref = std::make_unique <zw_value> ((*seq->get_seq ())[idx]->clone ());

  return ref.get ();
}
