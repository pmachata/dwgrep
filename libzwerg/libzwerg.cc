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

#include "libzwergP.hh"

#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

#include "builtin.hh"
#include "builtin-core.hh"
#include "op.hh"
#include "parser.hh"
#include "stack.hh"
#include "tree.hh"
#include "tree_cr.hh"

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
      stack->m_values.push_back (value->clone ());
      return true;
    }, false, out_err);
}

bool
zw_stack_push_take (zw_stack *stack, zw_value *value, zw_error **out_err)
{
  return capture_errors ([&] () {
      stack->m_values.push_back (std::unique_ptr <zw_value> (value));
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
      tree t = parse_query ({query, query_len});
      t.simplify ();

      layout l;
      auto origin = std::make_shared <op_origin> (l);
      auto op = t.build_exec (l, origin, *voc->m_voc);

      return new zw_query {l, *origin, op};
    }, nullptr, out_err);
}

void
zw_query_destroy (zw_query *query)
{
  delete query;
}

size_t
zw_value_pos (zw_value const *value)
{
  return value->get_pos ();
}

void
zw_value_destroy (zw_value *value)
{
  delete value;
}

zw_value *
zw_value_clone (zw_value const *val, size_t pos, zw_error **out_err)
{
  return capture_errors ([&] () {
      auto clone = val->clone ();
      assert (clone != nullptr);
      return clone.release ();
    }, nullptr, out_err);
}


zw_result *
zw_query_execute (zw_query const *query, zw_stack const *input_stack,
		  zw_error **out_err)
{
  return capture_errors ([&] () {
      auto stk = std::make_unique <stack> ();
      for (auto const &emt: input_stack->m_values)
	stk->push (emt->clone ());

      return new zw_result {query->m_l, query->m_origin, query->m_op,
			    std::move (stk)};
    }, nullptr, out_err);
}

bool
zw_result_next (zw_result *result, zw_stack **out_stack, zw_error **out_err)
{
  return capture_errors ([&] () {
      std::unique_ptr <stack> ret = result->next ();
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
	*it++ = ret->pop ();

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
  return val->is <value_cst> ();
}

bool
zw_value_is_str (zw_value const *val)
{
  return val->is <value_str> ();
}

bool
zw_value_is_seq (zw_value const *val)
{
  return val->is <value_seq> ();
}

namespace
{
  constant
  extract_constant (zw_value const *val)
  {
    assert (val != nullptr);
    value_cst const *cst = value::as <value_cst> (val);
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

namespace
{
  zw_value *
  format_constant (zw_value const *val, brevity brv, zw_error **out_err)
  {
    auto cst = extract_constant (val);
    std::stringstream ss;
    ss << constant {cst.value (), cst.dom (), brv};
    std::string const &s = ss.str ();
    return zw_value_init_str_len (s.c_str (), s.size (), 0, out_err);
  }
}

zw_value *
zw_value_const_format (zw_value const *val, zw_error **out_err)
{
  return format_constant (val, brevity::full, out_err);
}

zw_value *
zw_value_const_format_brief (zw_value const *val, zw_error **out_err)
{
  return format_constant (val, brevity::brief, out_err);
}

char const *
zw_cdom_name (zw_cdom const *cdom)
{
  assert (cdom != nullptr);
  return cdom->name ();
}

bool
zw_cdom_is_arith (zw_cdom const *cdom)
{
  assert (cdom != nullptr);
  return cdom->safe_arith ();
}

zw_cdom const *
zw_cdom_dec (void)
{
  return &dec_constant_dom;
}

zw_cdom const *
zw_cdom_hex (void)
{
  return &hex_constant_dom;
}

zw_cdom const *
zw_cdom_oct (void)
{
  return &oct_constant_dom;
}

zw_cdom const *
zw_cdom_bin (void)
{
  return &bin_constant_dom;
}

zw_cdom const *
zw_cdom_bool (void)
{
  return &bool_constant_dom;
}

namespace
{
  template <class T>
  zw_value *
  init_const (T i, zw_cdom const *dom, size_t pos, zw_error **out_err)
  {
    return capture_errors ([&] () {
	constant cst {i, dom};
	return new value_cst {cst, pos};
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
      return new value_str {str, pos};
    }, nullptr, out_err);
}

char const *
zw_value_str_str (zw_value const *val, size_t *lenp)
{
  assert (val != nullptr);
  assert (lenp != nullptr);

  value_str const &str = value::require_as <value_str> (val);
  *lenp = str.get_string ().length ();
  return str.get_string ().c_str ();
}

size_t
zw_value_seq_length (zw_value const *val)
{
  assert (val != nullptr);
  return value::require_as <value_seq> (val).get_seq ()->size ();
}

zw_value const *
zw_value_seq_at (zw_value const *val, size_t idx)
{
  assert (val != nullptr);

  value_seq const &seq = value::require_as <value_seq> (val);
  size_t sz = seq.get_seq ()->size ();
  assert (idx < sz);
  return (*seq.get_seq ())[idx].get ();
}
