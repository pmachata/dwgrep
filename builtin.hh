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

#ifndef _BUILTIN_H_
#define _BUILTIN_H_

#include <memory>
#include <string>

#include "dwgrep.hh"
#include "scope.hh"

struct pred;
struct op;

class builtin
{
public:
  virtual std::unique_ptr <pred>
  build_pred (dwgrep_graph::sptr q, std::shared_ptr <scope> scope) const;

  virtual std::shared_ptr <op>
  build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
	      std::shared_ptr <scope> scope) const;

  virtual char const *name () const = 0;
};

class pred_builtin
  : public builtin
{
protected:
  bool m_positive;

public:
  explicit pred_builtin (bool positive)
    : m_positive {positive}
  {}

  // Return either PRED, or PRED_NOT(PRED), depending on M_POSITIVE.
  std::unique_ptr <pred> maybe_invert (std::unique_ptr <pred> pred) const;
};

class builtin_dict
{
  struct builtins;
  std::unique_ptr <builtins> m_builtins;

public:
  builtin_dict ();
  builtin_dict (builtin_dict &a, builtin_dict &b);
  ~builtin_dict ();

  void add (std::shared_ptr <builtin const> b);
  void add (std::shared_ptr <builtin const> b, std::string const &name);
  std::shared_ptr <builtin const> find (std::string const &name) const;
};

void add_builtin_constant (builtin_dict &dict, constant cst, char const *name);

template <class T>
void
add_builtin_type_constant (builtin_dict &dict)
{
  add_builtin_constant (dict, value::get_type_const_of <T> (),
			T::vtype.name ());
}

#endif /* _BUILTIN_H_ */
