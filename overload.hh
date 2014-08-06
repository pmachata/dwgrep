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
#include <utility>

#include "op.hh"
#include "builtin.hh"
#include "selector.hh"

// Format an error message detailing what types a given operator
// needs.
void show_expects (std::string const &name, std::vector <selector> vts);

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
  std::vector <selector> m_selectors;
  std::vector <std::pair <std::shared_ptr <op_origin>,
			  std::shared_ptr <op>>> m_execs;
  std::vector <std::shared_ptr <pred>> m_preds;

public:
  overload_instance (std::vector
			<std::tuple <selector,
				     std::shared_ptr <builtin>>> const &stencil,
		     dwgrep_graph::sptr q, std::shared_ptr <scope> scope);

  std::pair <std::shared_ptr <op_origin>, std::shared_ptr <op>>
    find_exec (valfile &vf);

  std::shared_ptr <pred> find_pred (valfile &vf);

  void show_error (std::string const &name);
};

class overload_tab
{
  std::vector <std::tuple <selector, std::shared_ptr <builtin>>> m_overloads;

public:
  overload_tab () = default;
  overload_tab (overload_tab const &that) = default;
  overload_tab (overload_tab const &a, overload_tab const &b);

  void add_overload (selector vt, std::shared_ptr <builtin> b);

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
  add_overload (T::get_selector (),
		std::make_shared <overload_op_builtin <T>> ());
}

template <class T>
void
overload_tab::add_simple_pred_overload ()
{
  add_overload (T::get_selector (),
		std::make_shared <overload_pred_builtin <T>> ());
}

template <class... VT>
struct op_overload_impl
  : public stub_op
{
protected:
  template <class T>
  static auto collect1 (valfile &vf)
  {
    auto dv = vf.pop_as <T> ();
    assert (dv != nullptr);
    return std::make_tuple (std::move (dv));
  }

  template <size_t Fake>
  static auto collect (valfile &vf)
  {
    return std::tuple <> {};
  }

  template <size_t Fake, class T, class... Ts>
  static auto collect (valfile &vf)
  {
    auto rest = collect <Fake, Ts...> (vf);
    return std::tuple_cat (collect1 <T> (vf), std::move (rest));
  }

  dwgrep_graph::sptr m_gr;

public:
  op_overload_impl (std::shared_ptr <op> upstream, dwgrep_graph::sptr gr,
		    std::shared_ptr <scope> scope)
    : stub_op {upstream, gr, scope}
    , m_gr {gr}
  {}

  static selector get_selector ()
  { return {VT::vtype...}; }
};

template <class... VT>
struct op_overload
  : public op_overload_impl <VT...>
{
  template <size_t... I>
  std::unique_ptr <value>
  call_operate (std::index_sequence <I...>,
		std::tuple <std::unique_ptr <VT>...> args)
  {
    return operate (std::move (std::get <I> (args))...);
  }

public:
  typedef op_overload_impl <VT...> super_t;
  using super_t::super_t;

  valfile::uptr
  next () override final
  {
    while (auto vf = this->m_upstream->next ())
      if (auto nv = call_operate (std::index_sequence_for <VT...> {},
				  super_t::template collect <0, VT...> (*vf)))
	{
	  vf->push (std::move (nv));
	  return vf;
	}

    return nullptr;
  }

  virtual std::unique_ptr <value> operate (std::unique_ptr <VT>... vals) = 0;
};

template <class... VT>
struct op_yielding_overload
  : public op_overload_impl <VT...>
{
  typedef op_overload_impl <VT...> super_t;
  using super_t::super_t;

private:
  template <size_t... I>
  std::unique_ptr <value_producer>
  call_operate (std::index_sequence <I...>,
		std::tuple <std::unique_ptr <VT>...> args)
  {
    return operate (std::move (std::get <I> (args))...);
  }

  valfile::uptr m_vf;
  std::unique_ptr <value_producer> m_prod;

  void
  reset_me ()
  {
    m_prod = nullptr;
    m_vf = nullptr;
  }

public:
  valfile::uptr
  next () override final
  {
    while (true)
      {
	while (m_prod == nullptr)
	  if (auto vf = this->m_upstream->next ())
	    {
	      m_prod = call_operate
		(std::index_sequence_for <VT...> {},
		 super_t::template collect <0, VT...> (*vf));
	      m_vf = std::move (vf);
	    }
	  else
	    return nullptr;

	if (auto v = m_prod->next ())
	  {
	    auto ret = std::make_unique <valfile> (*m_vf);
	    ret->push (std::move (v));
	    return ret;
	  }

	reset_me ();
      }
  }

  void
  reset () override
  {
    reset_me ();
    super_t::reset ();
  }

  virtual std::unique_ptr <value_producer>
	operate (std::unique_ptr <VT>... vals) = 0;
};

#endif /* _OVERLOAD_H_ */
