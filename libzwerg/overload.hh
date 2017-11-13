/*
   Copyright (C) 2017 Petr Machata
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
#include "std-memory.hh"
#include "std-utility.hh"
#include <iostream>

#include "op.hh"
#include "builtin.hh"
#include "selector.hh"

// Some operators are generically applicable.  In order to allow
// adding new value types, and reuse the same operators for them, this
// overloading infrastructure is provided.
//
// Every overload, i.e. a specialization of the operator to a type, is
// a builtin.  The op that this builtin creates contains the code that
// handles the type that this overload is specialized to.  Most of the
// time, both the op and the bulitin that wraps it are boring and
// predictable.  You will most likely want to inherit the op off
// either of op_overload, op_yielding_overload or pred_overload.
//
// These individual overloads are collected into an overload table,
// which maps value types to builtins.
//
// The overarching operator is then a builtin as well.  It creates an
// op (or a pred) that inherits off overload_op (overload_pred), which
// handles the dispatching itself.  Constructor of that class needs an
// overload instance, which is created from overload table above.
//
// As an additional simplification, there's some templating magic that
// sets up stage just like the above expects it: op_overload and
// pred_overload.  Most of the time, all you need to write are
// sub-classes of these templates.  See below for comments on these.
//
// For example of this in action, see e.g. operator length.

class overload_instance
{
  std::vector <selector> m_selectors;
  std::vector <std::tuple <std::shared_ptr <op_origin>,
			   std::shared_ptr <op>,
			   size_t>> m_execs;
  std::vector <std::shared_ptr <pred>> m_preds;

public:
  overload_instance (std::vector
			<std::tuple <selector,
				     std::shared_ptr <builtin>>> const &stencil);

  size_t max_reserve () const;
  std::tuple <op_origin *, op *, size_t> find_exec (stack &stk) const;
  std::shared_ptr <pred> find_pred (stack &stk) const;

  void show_error (std::string const &name, selector profile) const;
};

struct overload_tab
{
  using overload_vec = std::vector <std::tuple <selector,
						std::shared_ptr <builtin>>>;

private:
  overload_vec m_overloads;

public:
  overload_tab () = default;
  overload_tab (overload_tab const &that) = default;
  overload_tab (overload_tab const &a, overload_tab const &b);

  void add_overload (selector vt, std::shared_ptr <builtin> b);

  template <class T, class... As> void add_op_overload (As &&... arg);
  template <class T, class... As> void add_pred_overload (As &&... arg);

  overload_instance instantiate ();
  overload_vec const &get_overloads () const { return m_overloads; }
};

class overload_op
  : public op
{
  class state;
  std::shared_ptr <op> m_upstream;
  overload_instance m_ovl_inst;

public:
  overload_op (std::shared_ptr <op> upstream, overload_instance ovl_inst);

  size_t reserve () const override;
  void state_con (void *buf) const override;
  void state_des (void *buf) const override;
  stack::uptr next (void *buf) const override final;
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

  pred_result result (stack &stk) override final;
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

  char const *name () const override final
  { return m_name; }

  std::string docstring () const override final;

  virtual std::shared_ptr <overloaded_builtin>
  create_merged (std::shared_ptr <overload_tab> tab) const = 0;
};

// Base class for overloaded operation builtins.
struct overloaded_op_builtin
  : public overloaded_builtin
{
  using overloaded_builtin::overloaded_builtin;

  std::shared_ptr <op> build_exec (std::shared_ptr <op> upstream)
    const override final;

  std::shared_ptr <overloaded_builtin>
  create_merged (std::shared_ptr <overload_tab> tab) const override final;
};

// Base class for overloaded predicate builtins.
struct overloaded_pred_builtin
  : public overloaded_builtin
{
  bool m_positive;
  overloaded_pred_builtin (char const *name,
			   std::shared_ptr <overload_tab> ovl_tab,
			   bool positive)
    : overloaded_builtin {name, ovl_tab}
    , m_positive {positive}
  {}

  std::unique_ptr <pred> build_pred () const override final;

  std::shared_ptr <overloaded_builtin>
  create_merged (std::shared_ptr <overload_tab> tab) const override final;
};


// The following is for delayed dispatch of an op constructor with
// arguments provided when adding an overload through add_op_overload.

template <class Op, class... Args>
struct overload_op_builder_impl
{
  template <size_t... I>
  static std::shared_ptr <op>
  build (std::shared_ptr <op> upstream,
	 std::index_sequence <I...>,
	 std::tuple <typename std::remove_reference <Args>::type...>
	 	const &args)
  {
    return std::make_shared <Op> (upstream, std::get <I> (args)...);
  }
};

template <class Op, class... Args>
void
overload_tab::add_op_overload (Args &&... args)
{
  struct overload_op_builtin
    : public builtin
  {
    std::tuple <typename std::remove_reference <Args>::type...> m_args;

    overload_op_builtin (Args &&... args2)
      : m_args (std::forward <Args> (args2)...)
    {}

    std::shared_ptr <op>
    build_exec (std::shared_ptr <op> upstream) const override final
    {
      return overload_op_builder_impl <Op, Args...>::template build
	(upstream, std::index_sequence_for <Args...> {}, m_args);
    }

    char const *
    name () const override final
    {
      return "overload";
    }

    std::string
    docstring () const override
    {
      return Op::docstring ();
    }

    builtin_protomap
    protomap () const override
    {
      return Op::protomap ();
    }
  };

  add_overload (Op::get_selector (),
		std::make_shared <overload_op_builtin>
			(std::forward <Args> (args)...));
}


// The following is for delayed dispatch of a pred constructor with
// arguments provided when adding an overload through
// add_pred_overload.

template <class Pred, class... Args>
struct overload_pred_builder_impl
{
  template <size_t... I>
  static std::unique_ptr <pred>
  build (std::index_sequence <I...>,
	 std::tuple <typename std::remove_reference <Args>::type...>
	 	const &args)
  {
    return std::make_unique <Pred> (std::get <I> (args)...);
  }
};

template <class Pred, class... Args>
void
overload_tab::add_pred_overload (Args &&... args)
{
  struct overload_pred_builtin
    : public builtin
  {
    std::tuple <typename std::remove_reference <Args>::type...> const m_args;

    overload_pred_builtin (Args... args2)
      : m_args (args2...)
    {}

    std::unique_ptr <pred>
    build_pred () const override final
    {
      return overload_pred_builder_impl <Pred, Args...>::template build
        (std::index_sequence_for <Args...> {}, m_args);
    }

    char const *
    name () const override final
    {
      return "overload";
    }

    std::string
    docstring () const override
    {
      return Pred::docstring ();
    }

    builtin_protomap
    protomap () const override
    {
      return Pred::protomap ();
    }
  };

  add_overload (Pred::get_selector (),
		std::make_shared <overload_pred_builtin> (args...));
}

// Generators for type-safe overloads.
//
// While it is possible to hand-code each overload as a full-fledged
// op, this is often unnecessarily laborious and prone to errors.
// Instead, three class templates are provided for making this work
// simple:
//
//  - op_overload -- for simple overloads
//  - op_yielding_overload -- for overloads that yield more than once
//  - pred_overload -- for predicates
//
// The general usage is that you inherit off this template, and as
// arguments, you provide the types of values that you expect on stack
// (the last argument is TOS, deeper stack elements are to the left).
// The template sets up the necessary popping and argument gathering,
// and then calls a virtual with those arguments.  You provide
// implementation of this virtual.
//
// Return type of this virtual depends on the template type:
//
//  - op_overload: a value to be pushed
//
//  - op_yielding_overload: a value_producer that enumerates the
//    values that should become TOS of new stacks
//
//  - pred_overload: a pred_result

template <class... VT>
struct op_overload_impl
{
protected:
  template <class T>
  static std::tuple <std::unique_ptr <T>> collect1 (stack &stk)
  {
    auto dv = stk.pop_as <T> ();
    assert (dv != nullptr);
    return std::make_tuple (std::move (dv));
  }

  template <size_t Fake>
  static std::tuple <> collect (stack &stk)
  {
    return std::tuple <> {};
  }

  template <size_t Fake, class T, class... Ts>
  static std::tuple <std::unique_ptr <T>,
		     std::unique_ptr <Ts>...> collect (stack &stk)
  {
    auto rest = collect <Fake, Ts...> (stk);
    return std::tuple_cat (collect1 <T> (stk), std::move (rest));
  }

public:
  static selector get_selector ()
  { return {VT::vtype...}; }

  // This implementation is picked if the sub-class doesn't overload
  // the docstring () static itself.
  static std::string docstring ()
  { return ""; }
};

template <class RT, class... VT>
struct op_overload
  : public op_overload_impl <VT...>
  , public stub_op
{
  template <size_t... I>
  std::unique_ptr <RT>
  call_operate (std::index_sequence <I...>,
		std::tuple <std::unique_ptr <VT>...> args) const
  {
    return operate (std::move (std::get <I> (args))...);
  }

public:
  op_overload (std::shared_ptr <op> upstream)
    : stub_op {upstream, 0}
  {}

  stack::uptr
  next (void *buf) const override final
  {
    while (auto stk = this->m_upstream->next (buf))
      if (auto nv = call_operate
		(std::index_sequence_for <VT...> {},
		 op_overload_impl <VT...>::template collect <0, VT...> (*stk)))
	{
	  stk->push (std::move (nv));
	  return stk;
	}

    return nullptr;
  }

  virtual std::unique_ptr <RT> operate (std::unique_ptr <VT>... vals) const = 0;

  static builtin_protomap
  protomap ()
  {
    return {
      builtin_prototype ({VT::vtype...}, yield::maybe, {RT::vtype}),
    };
  }
};

template <class RT, class... VT>
struct op_once_overload
  : public op_overload_impl <VT...>
  , public stub_op
{
  template <size_t... I>
  RT
  call_operate (std::index_sequence <I...>,
		std::tuple <std::unique_ptr <VT>...> args) const
  {
    return operate (std::move (std::get <I> (args))...);
  }

public:
  op_once_overload (std::shared_ptr <op> upstream)
    : stub_op {upstream, 0}
  {}

  stack::uptr
  next (void *buf) const override final
  {
    if (auto stk = this->m_upstream->next (buf))
      {
	auto ret = call_operate
		(std::index_sequence_for <VT...> {},
		 op_overload_impl <VT...>::template collect <0, VT...> (*stk));
	stk->push (std::make_unique <RT> (std::move (ret)));
	return stk;
      }

    return nullptr;
  }

  virtual RT operate (std::unique_ptr <VT>... vals) const = 0;

  static builtin_protomap
  protomap ()
  {
    return {
      builtin_prototype ({VT::vtype...}, yield::once, {RT::vtype}),
    };
  }
};

template <class RT, class... VT>
class op_yielding_overload
  : public op_overload_impl <VT...>
  , public stub_op
{
  template <size_t... I>
  std::unique_ptr <value_producer <RT>>
  call_operate (std::index_sequence <I...>,
		std::tuple <std::unique_ptr <VT>...> args)
  {
    return operate (std::move (std::get <I> (args))...);
  }

  stack::uptr m_stk;
  std::unique_ptr <value_producer <RT>> m_prod;

  void
  reset_me ()
  {
    m_prod = nullptr;
    m_stk = nullptr;
  }

public:
  op_yielding_overload (std::shared_ptr <op> upstream)
    : stub_op {upstream}
  {}

  stack::uptr
  next () override final
  {
    while (true)
      {
	while (m_prod == nullptr)
	  if (auto stk = this->m_upstream->next ())
	    {
	      m_prod = call_operate
		(std::index_sequence_for <VT...> {},
		 op_overload_impl <VT...>::template collect <0, VT...> (*stk));
	      m_stk = std::move (stk);
	    }
	  else
	    return nullptr;

	if (auto v = m_prod->next ())
	  {
	    auto ret = std::make_unique <stack> (*m_stk);
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
    stub_op::reset ();
  }

  virtual std::unique_ptr <value_producer <RT>>
	operate (std::unique_ptr <VT>... vals) = 0;

  static builtin_protomap
  protomap ()
  {
    return {
      builtin_prototype ({VT::vtype...}, yield::many, {RT::vtype}),
    };
  }
};

template <class... VT>
struct pred_overload
  : public stub_pred
{
  template <class T>
  static std::tuple <T &> collect1 (stack &stk, size_t depth)
  {
    auto dv = stk.get_as <T> (depth);
    assert (dv != nullptr);
    return std::tuple <T &> (*dv);
  }

  template <size_t N>
  static std::tuple <> collect (stack &stk)
  {
    return std::tuple <> {};
  }

  template <size_t N, class T, class... Ts>
  static std::tuple <T &, Ts &...> collect (stack &stk)
  {
    auto rest = collect <N - 1, Ts...> (stk);
    return std::tuple_cat (collect1 <T> (stk, N - 1), rest);
  }

  template <size_t... I>
  pred_result
  call_result (std::index_sequence <I...>, std::tuple <VT &...> args)
  {
    return result (std::get <I> (args)...);
  }

public:
  pred_result
  result (stack &stk) override
  {
    return call_result (std::index_sequence_for <VT...> {},
			collect <sizeof... (VT), VT...> (stk));
  }

  virtual pred_result result (VT &... vals) = 0;

  static selector get_selector ()
  { return {VT::vtype...}; }

  static builtin_protomap
  protomap ()
  {
    return {
      builtin_prototype ({VT::vtype...}, yield::pred, {}),
    };
  }

  // This implementation is picked if the sub-class doesn't overload
  // the docstring () static itself.
  static std::string docstring ()
  { return ""; }
};

#endif /* _OVERLOAD_H_ */
