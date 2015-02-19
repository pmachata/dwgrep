/*
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
  std::shared_ptr <frame> m_frame;

public:
  static value_type const vtype;

  value_closure (tree const &t, std::shared_ptr <frame> frame, size_t pos);
  value_closure (value_closure const &that);
  ~value_closure();

  tree const &get_tree () const
  { return *m_t; }

  std::shared_ptr <frame> get_frame () const
  { return m_frame; }

  void show (std::ostream &o) const override;
  std::unique_ptr <value> clone () const override;
  cmp_result cmp (value const &that) const override;

  // Destruction of stack frames is actually fairly involved.
  // Consider:
  //
  //   '{} dup -> F G;'
  //
  // The current stack frame holds the closure associated with F and
  // G, and that closure holds the stack frame again in case it needs
  // to look up variables in surrounding context.  So we end up with a
  // circular dependency.
  //
  // Note that neither of these shared pointers can be made weak.
  // Closures need to hold on the surrounding context strongly,
  // because in general it won't be the same frame as we are running
  // in, and they will be the only one to keep that context alive.
  // And frame naturally needs to hold its variables alive, nobody
  // else can.
  //
  // So this routine checks this case explicitly.  If anyone but F now
  // holds the reference, and all the references come back from the
  // frame itself, sever those inner references so that the frame
  // garbage-collects naturally when we leave the scope.
  //
  // This needs to be called strategically at points where
  // participants of the reference cycle are lost track of.
  static void maybe_unlink_frame (std::shared_ptr <frame> &f);
};

#endif /* _VALUE_CLOSURE_H_ */
