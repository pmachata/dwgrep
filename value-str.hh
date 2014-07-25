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

#ifndef _VALUE_STR_H_
#define _VALUE_STR_H_

#include <vector>
#include "value.hh"
#include "op.hh"

class value_str
  : public value
{
  std::string m_str;

public:
  static value_type const vtype;

  value_str (std::string &&str, size_t pos)
    : value {vtype, pos}
    , m_str {std::move (str)}
  {}

  std::string const &get_string () const
  { return m_str; }

  void show (std::ostream &o, bool full) const override;
  std::unique_ptr <value> clone () const override;
  cmp_result cmp (value const &that) const override;
};

struct op_add_str
  : public stub_op
{
  using stub_op::stub_op;

  static value_type get_value_type ()
  { return value_str::vtype; }

  valfile::uptr next () override;
};

struct op_length_str
  : public stub_op
{
  using stub_op::stub_op;

  static value_type get_value_type ()
  { return value_str::vtype; }

  valfile::uptr next () override;
};

struct op_elem_str
  : public inner_op
{
  struct state;
  std::unique_ptr <state> m_state;

  op_elem_str (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
	      std::shared_ptr <scope> scope);
  ~op_elem_str ();

  valfile::uptr next () override;
  std::string name () const override;
  void reset () override;

  static value_type get_value_type ()
  { return value_str::vtype; }
};

struct pred_empty_str
  : public stub_pred
{
  using stub_pred::stub_pred;

  static value_type get_value_type ()
  { return value_str::vtype; }

  pred_result result (valfile &vf) override;
};

struct pred_find_str
  : public stub_pred
{
  using stub_pred::stub_pred;

  static value_type get_value_type ()
  { return value_str::vtype; }

  pred_result result (valfile &vf) override;
};

class builtin_match
  : public pred_builtin
{
  struct p;

public:
  using pred_builtin::pred_builtin;

  std::unique_ptr <pred> build_pred (dwgrep_graph::sptr q,
				     std::shared_ptr <scope> scope)
    const override;

  char const *name () const override;
};

#endif /* _VALUE_STR_H_ */
