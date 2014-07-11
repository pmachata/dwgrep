#ifndef _OVERLOAD_H_
#define _OVERLOAD_H_

#include <vector>
#include <tuple>
#include <memory>

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
				   std::shared_ptr <op>>> overload_vec;
  overload_vec m_overloads;

public:
  overload_instance (std::vector <std::tuple <value_type,
					      builtin &>> const &stencil,
		     dwgrep_graph::sptr q, std::shared_ptr <scope> scope);

  overload_vec::value_type *find_overload (value_type vt);

  void show_error (std::string const &name);
};

class overload_tab
{
  std::vector <std::tuple <value_type, builtin &>> m_overloads;

public:
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

template <class OP>
struct overload_builtin
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

#endif /* _OVERLOAD_H_ */
