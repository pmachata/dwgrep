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

#include "libzwergP.hh"

#include <cstring>
#include <iostream>
#include <string>

#include "builtin-dw.hh"
#include "builtin.hh"
#include "init.hh"
#include "op.hh"
#include "parser.hh"
#include "stack.hh"
#include "tree.hh"
#include "value-dw.hh"

namespace
{
  zw_error *
  zw_error_new (char const *message)
  {
    return new zw_error {message};
  }

  template <class U>
  U
  allocate_error (char const *message, U fail_return, zw_error **out_err)
  {
    assert (out_err != nullptr);
    try
      {
	*out_err = zw_error_new (message);
	return fail_return;
      }
    catch (std::exception const &exc)
      {
	std::cerr << "Error: " << message << "\n"
		  << "There was an error while handling that error: "
		  << exc.what () << "\n"
		  << "Aborting.\n";
	std::abort ();
      }
    catch (...)
      {
	std::cerr << "Error: " << message << "\n"
		  << "There was an unknown error while handling that error.\n"
		  << "Aborting.\n";
	std::abort ();
      }
  }

  template <class T>
  auto
  capture_errors (T &&callback, decltype (callback ()) fail_return,
		  zw_error **out_err) -> decltype (callback ())
  {
    try
      {
	return callback ();
      }
    catch (std::exception const &exc)
      {
	return allocate_error (exc.what (), fail_return, out_err);
      }
    catch (...)
      {
	return allocate_error ("unknown error", fail_return, out_err);
      }
  }
}


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

extern "C" zw_vocabulary const *
zw_vocabulary_dwarf (zw_error **out_err)
{
  return capture_errors ([&] () {
      static zw_vocabulary v {dwgrep_vocabulary_dw ()};
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
	(std::make_unique <zw_value> (value->m_value));
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

bool
zw_stack_dump_xxx (zw_stack const *stack, zw_error **out_err)
{
  return capture_errors ([&] () {
      auto const &values = stack->m_values;
      for (auto it = values.rbegin (); it != values.rend (); ++it)
	std::cout << *((*it)->m_value) << std::endl;
      return true;
    }, false, out_err);
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

namespace
{
  zw_value *
  init_dwarf (char const *filename, doneness d, size_t pos, zw_error **out_err)
  {
    return capture_errors ([&] () {
	return new zw_value {std::make_unique <value_dwarf> (filename, pos, d)};
      }, nullptr, out_err);
  }
}

zw_value *
zw_value_init_dwarf (char const *filename, size_t pos, zw_error **out_err)
{
  return init_dwarf (filename, doneness::cooked, pos, out_err);
}

zw_value *
zw_value_init_dwarf_raw (char const *filename, size_t pos, zw_error **out_err)
{
  return init_dwarf (filename, doneness::raw, pos, out_err);
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
	stk->push (emt->m_value);
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
