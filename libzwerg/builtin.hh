/*
   Copyright (C) 2017, 2018 Petr Machata
   Copyright (C) 2014, 2015 Red Hat, Inc.
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

#ifndef _BUILTIN_H_
#define _BUILTIN_H_

#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "constant.hh"
#include "layout.hh"
#include "value.hh"

struct pred;
struct op;

enum class yield
  {
    maybe,	// The operator may yield once, or not at all.
    once,	// The operator yields exactly once.
    many,	// The operator yields zero or more times.
    pred,	// Like maybe, but in addition, the builtin is a
		// predicate.  The input arguments are not consumed,
		// and output arguments are ignored.
  };

// Prototype map for builtins.  A vector of prototype rules.  Each
// rule reads as follows:
//
//   - <1>: A vector of value types near TOS.  rbegin represents TOS.
//
//   - <2>: A yield declaration (see above).
//
//   - <3>: A vector of value types left on stack.  rbegin is what
//     will be on TOS after the operator is done.
//
// An empty map implicitly means "who knows".
using builtin_prototype = std::tuple <std::vector <value_type>, yield,
				      std::vector <value_type>>;
using builtin_protomap = std::vector <builtin_prototype>;

class builtin
{
public:
  virtual ~builtin () {}

  virtual std::unique_ptr <pred>
  build_pred (layout &l) const;

  virtual std::shared_ptr <op>
  build_exec (layout &l, std::shared_ptr <op> upstream) const;

  virtual char const *name () const = 0;

  virtual std::string docstring () const;
  virtual builtin_protomap protomap () const;
};

// Return either PRED, or PRED_NOT(PRED), depending on POSITIVE.
std::unique_ptr <pred> maybe_invert (std::unique_ptr <pred> pred,
				     bool positive);

class pred_builtin
  : public builtin
{
protected:
  bool m_positive;

public:
  explicit pred_builtin (bool positive)
    : m_positive {positive}
  {}
};

struct vocabulary
{
  using builtin_map = std::map <std::string, std::shared_ptr <builtin const>>;

private:
  builtin_map m_builtins;

public:
  vocabulary ();
  vocabulary (vocabulary const &a, vocabulary const &b);
  ~vocabulary ();

  void add (std::shared_ptr <builtin const> b);
  void add (std::shared_ptr <builtin const> b, std::string const &name);
  std::shared_ptr <builtin const> find (std::string const &name) const;

  builtin_map const &get_builtins () const
  { return m_builtins; }
};

void add_builtin_constant (vocabulary &voc, constant cst, char const *name);

template <class T>
void
add_builtin_type_constant (vocabulary &voc)
{
  add_builtin_constant (voc, value::get_type_const_of <T> (),
			T::vtype.name ());
}

template <class Op, class... Args>
class simple_exec_builtin
  : public builtin
{
  char const *m_name;
  std::tuple <Args...> m_args;

  template <size_t... I>
  static std::shared_ptr <op>
  build (std::shared_ptr <op> upstream,
	 std::index_sequence <I...>,
	 std::tuple <typename std::remove_reference <Args>::type...>
	 	const &args)
  {
    return std::make_shared <Op> (upstream, std::get <I> (args)...);
  }

public:
  simple_exec_builtin (char const *name, Args... args)
    : m_name {name}
    , m_args {args...}
  {}

  std::shared_ptr <op>
  build_exec (layout &l, std::shared_ptr <op> upstream) const override final
  {
    return build (upstream, std::index_sequence_for <Args...> {}, m_args);
  }

  char const *
  name () const override final
  {
    return m_name;
  }

  std::string
  docstring () const override
  {
    return Op::docstring ();
  }
};

#endif /* _BUILTIN_H_ */
