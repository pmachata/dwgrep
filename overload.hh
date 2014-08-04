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

#ifndef _OVERLOAD_H_
#define _OVERLOAD_H_

#include <vector>
#include <tuple>
#include <memory>

#include "op.hh"
#include "builtin.hh"

// Format an error message detailing what types a given operator
// needs.
void show_expects (std::string const &name, std::vector <value_type> vts);

// Some operators are generically applicable.  In order to allow
// adding new value types, and reuse the same operators for them, this
// overloading infrastructure is provided.
//
// Every overload, i.e. a specialization of the operator to a type, is
// a builtin.  The op that this builtin creates contains the code that
// handles the type that this overload is specialized to.  Most of the
// time, both the op and the bulitin that wraps it are boring and
// predictable.  You will most likely want to inherit the op off
// stub_op, and then create a simple builtin by creating an object of
// type overload_builtin <your_op>.
//
// These individual overloads are collected into an overload table,
// which maps value types to builtins.  The table itself needs to be
// public, so that other modules can add their specializations in.
//
// The overaching operator is then a builtin as well.  It creates an
// op that inherits off overload_op, which handles the dispatching
// itself.  Constructor of that class needs an overload instance,
// which is created from overload table above.
//
// For example of this in action, see e.g. operator length.

class overload_instance
{
  typedef std::vector <std::tuple <value_type,
				   std::shared_ptr <op_origin>,
				   std::shared_ptr <op>>> exec_vec;
  exec_vec m_execs;

  typedef std::vector <std::tuple <value_type,
				   std::shared_ptr <pred>>> pred_vec;
  pred_vec m_preds;

  unsigned m_arity;

public:
  overload_instance (std::vector
			<std::tuple <value_type,
				     std::shared_ptr <builtin>>> const &stencil,
		     dwgrep_graph::sptr q, std::shared_ptr <scope> scope,
		     unsigned arity);

  exec_vec::value_type *find_exec (valfile &vf);
  pred_vec::value_type *find_pred (valfile &vf);

  void show_error (std::string const &name);
};

class overload_tab
{
  unsigned m_arity;
  std::vector <std::tuple <value_type, std::shared_ptr <builtin>>> m_overloads;

public:
  explicit overload_tab (unsigned arity = 1)
    : m_arity {arity}
  {}

  overload_tab (overload_tab const &a, overload_tab const &b);

  void add_overload (value_type vt, std::shared_ptr <builtin> b);

  template <class T> void add_simple_op_overload ();
  template <class T> void add_simple_pred_overload ();

  overload_instance instantiate (dwgrep_graph::sptr q,
				 std::shared_ptr <scope> scope);
};

class overload_op
  : public op
{
  class pimpl;
  std::unique_ptr <pimpl> m_pimpl;

public:
  overload_op (std::shared_ptr <op> upstream, overload_instance ovl_inst);
  ~overload_op ();

  valfile::uptr next () override final;
  void reset () override final;
};

class overload_pred
  : public pred
{
  overload_instance m_ovl_inst;
public:
  overload_pred (overload_instance ovl_inst)
    : m_ovl_inst {ovl_inst}
  {}

  void
  reset () override final
  {}

  pred_result result (valfile &vf) override final;
};

// Base class for overloaded builtins.
class overloaded_builtin
  : public builtin
{
  char const *m_name;
  std::shared_ptr <overload_tab> m_ovl_tab;

public:
  overloaded_builtin (char const *name,
		      std::shared_ptr <overload_tab> ovl_tab)
    : m_name {name}
    , m_ovl_tab {ovl_tab}
  {}

  explicit overloaded_builtin (char const *name)
    : overloaded_builtin {name, std::make_shared <overload_tab> ()}
  {}

  std::shared_ptr <overload_tab> get_overload_tab () const
  { return m_ovl_tab; }

  char const *name () const override final { return m_name; }

  virtual std::shared_ptr <overloaded_builtin>
  create_merged (std::shared_ptr <overload_tab> tab) const = 0;
};

// Base class for overloaded operation builtins.
struct overloaded_op_builtin
  : public overloaded_builtin
{
  using overloaded_builtin::overloaded_builtin;

  std::shared_ptr <op> build_exec (std::shared_ptr <op> upstream,
				   dwgrep_graph::sptr q,
				   std::shared_ptr <scope> scope)
    const override final;

  std::shared_ptr <overloaded_builtin>
  create_merged (std::shared_ptr <overload_tab> tab) const override final;
};

// Base class for overloaded predicate builtins.
struct overloaded_pred_builtin
  : public overloaded_builtin
{
  using overloaded_builtin::overloaded_builtin;

  std::unique_ptr <pred> build_pred (dwgrep_graph::sptr q,
				     std::shared_ptr <scope> scope)
    const override final;

  std::shared_ptr <overloaded_builtin>
  create_merged (std::shared_ptr <overload_tab> tab) const override final;
};

// Base class for individual overloads that produce an op.
template <class OP>
struct overload_op_builtin
  : public builtin
{
  std::shared_ptr <op>
  build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
	      std::shared_ptr <scope> scope) const override final
  {
    return std::make_shared <OP> (upstream, q, scope);
  }

  char const *
  name () const override final
  {
    return "overload";
  }
};

// Base class for individual overloads that produce a pred.
template <class PRED>
struct overload_pred_builtin
  : public builtin
{
  std::unique_ptr <pred>
  build_pred (dwgrep_graph::sptr q,
	      std::shared_ptr <scope> scope) const override final
  {
    return std::make_unique <PRED> (q, scope);
  }

  char const *
  name () const override final
  {
    return "overload";
  }
};

template <class T>
void
overload_tab::add_simple_op_overload ()
{
  add_overload (T::get_value_type (),
		std::make_shared <overload_op_builtin <T>> ());
}

template <class T>
void
overload_tab::add_simple_pred_overload ()
{
  add_overload (T::get_value_type (),
		std::make_shared <overload_pred_builtin <T>> ());
}

template <class VT>
struct op_unary_overload
  : public stub_op
{
protected:
  dwgrep_graph::sptr m_gr;

public:
  op_unary_overload (std::shared_ptr <op> upstream, dwgrep_graph::sptr gr,
		     std::shared_ptr <scope> scope)
    : stub_op {upstream, gr, scope}
    , m_gr {gr}
  {}

  static value_type get_value_type ()
  { return VT::vtype; }

  valfile::uptr next () override final
  {
    while (auto vf = m_upstream->next ())
      {
	auto dv = vf->pop_as <VT> ();
	assert (dv != nullptr);
	if (auto nv = operate (std::move (dv)))
	  {
	    vf->push (std::move (nv));
	    return vf;
	  }
      }
    return nullptr;
  }

  virtual std::unique_ptr <value> operate (std::unique_ptr <VT> val) = 0;
};

#endif /* _OVERLOAD_H_ */
