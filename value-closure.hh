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

#ifndef _VALUE_CLOSURE_H_
#define _VALUE_CLOSURE_H_

#include "value.hh"

class tree;
class frame;
class scope;

class value_closure
  : public value
{
  std::unique_ptr <tree> m_t;
  std::shared_ptr <scope> m_scope;
  std::shared_ptr <frame> m_frame;

public:
  static value_type const vtype;

  value_closure (tree const &t, std::shared_ptr <scope> scope,
		 std::shared_ptr <frame> frame, size_t pos);
  value_closure (value_closure const &that);
  ~value_closure();

  tree const &get_tree () const
  { return *m_t; }

  std::shared_ptr <scope> get_scope () const
  { return m_scope; }

  std::shared_ptr <frame> get_frame () const
  { return m_frame; }

  void show (std::ostream &o, brevity brv) const override;
  std::unique_ptr <value> clone () const override;
  cmp_result cmp (value const &that) const override;
};

#endif /* _VALUE_CLOSURE_H_ */
