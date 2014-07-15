#ifndef _OVERLOAD_H_
#define _OVERLOAD_H_

#include <vector>
#include <tuple>
#include <memory>
#include "make_unique.hh"

#include "op.hh"
#include "builtin.hh"

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
  overload_instance (std::vector <std::tuple <value_type,
					      builtin &>> const &stencil,
		     dwgrep_graph::sptr q, std::shared_ptr <scope> scope,
		     unsigned arity);

  exec_vec::value_type *find_exec (valfile &vf);
  pred_vec::value_type *find_pred (valfile &vf);

  void show_error (std::string const &name);
};

class overload_tab
{
  unsigned m_arity;
  std::vector <std::tuple <value_type, builtin &>> m_overloads;

public:
  explicit overload_tab (unsigned arity = 1)
    : m_arity {arity}
  {}

  void add_overload (value_type vt, builtin &b);

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

template <class OP>
struct overload_op_builtin
  : public builtin
{
  std::shared_ptr <op>
  build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
	      std::shared_ptr <scope> scope) const override final
  {
    return std::make_shared <OP> (upstream);
  }

  char const *
  name () const override final
  {
    return "overload";
  }
};

template <class PRED>
struct overload_pred_builtin
  : public builtin
{
  std::unique_ptr <pred>
  build_pred (dwgrep_graph::sptr q,
	      std::shared_ptr <scope> scope) const override final
  {
    return std::make_unique <PRED> ();
  }

  char const *
  name () const override final
  {
    return "overload";
  }
};

#endif /* _OVERLOAD_H_ */
