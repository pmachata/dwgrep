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

#ifndef _TREE_CR_H_
#define _TREE_CR_H_

#include "tree.hh"
#include "make_unique.hh"

template <tree_type TT>
tree *
tree::create_nullary ()
{
  static_assert (tree_arity <TT>::value == tree_arity_v::NULLARY,
		 "Wrong tree arity.");
  return new tree {TT};
}

template <tree_type TT>
tree *
tree::create_unary (tree *op)
{
  static_assert (tree_arity <TT>::value == tree_arity_v::UNARY,
		 "Wrong tree arity.");
  auto t = new tree {TT};
  t->take_child (op);
  return t;
}

template <tree_type TT>
tree *
tree::create_binary (tree *lhs, tree *rhs)
{
  static_assert (tree_arity <TT>::value == tree_arity_v::BINARY,
		 "Wrong tree arity.");
  auto t = new tree {TT};
  t->take_child (lhs);
  t->take_child (rhs);
  return t;
}

template <tree_type TT>
tree *
tree::create_ternary (tree *op1, tree *op2, tree *op3)
{
  static_assert (tree_arity <TT>::value == tree_arity_v::TERNARY,
		 "Wrong tree arity.");
  auto t = new tree {TT};
  t->take_child (op1);
  t->take_child (op2);
  t->take_child (op3);
  return t;
}

template <tree_type TT>
tree *
tree::create_str (std::string s)
{
  static_assert (tree_arity <TT>::value == tree_arity_v::STR,
		 "Wrong tree arity.");
  auto t = new tree {TT};
  t->m_str = std::make_unique <std::string> (std::move (s));
  return t;
}

template <tree_type TT>
tree *
tree::create_const (constant c)
{
  static_assert (tree_arity <TT>::value == tree_arity_v::CST,
		 "Wrong tree arity.");
  auto t = new tree {TT};
  t->m_cst = std::make_unique <constant> (std::move (c));
  return t;
}

// Creates either a CAT or an ALT node.
//
// It is smart in that it transforms CAT's of CAT's into one
// overarching CAT, and appends or prepends other nodes to an
// existing CAT if possible.  It also knows to ignore a nullptr
// tree.
template <tree_type TT>
tree *
tree::create_cat (tree *t1, tree *t2)
{
  bool cat1 = t1 != nullptr && t1->m_tt == TT;
  bool cat2 = t2 != nullptr && t2->m_tt == TT;

  if (cat1 && cat2)
    {
      t1->take_cat (t2);
      return t1;
    }
  else if (cat1 && t2 != nullptr)
    {
      t1->take_child (t2);
      return t1;
    }
  else if (cat2 && t1 != nullptr)
    {
      t2->take_child_front (t1);
      return t2;
    }

  if (t1 == nullptr)
    {
      assert (t2 != nullptr);
      return t2;
    }
  else if (t2 == nullptr)
    return t1;
  else
    return tree::create_binary <TT> (t1, t2);
}

#endif /* _TREE_CR_H_ */
