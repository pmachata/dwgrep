/*
   Copyright (C) 2017 Petr Machata
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

#ifndef _TREE_H_
#define _TREE_H_

#include <vector>
#include <string>
#include <iosfwd>

#include "constant.hh"
#include "stack.hh"
#include "builtin.hh"
#include "layout.hh"

// These constants describe how a tree is allowed to be constructed.
// It's mostly present to make sure we don't inadvertently construct
// e.g. a CONST tree with no associated constant.
//
// NULLARY, UNARY, BINARY -- a tree that's constructed over zero, one
// or two sub-trees.  Often that's the only number of sub-trees that
// makes sense for given tree type, but CAT and ALT can hold arbitrary
// number of sub-trees, they are just constructed from two.  FORMAT is
// NULLARY, but holds any number of sub-trees as well.
//
// STR, CONST -- A tree that holds a constant, has no children.
//
// SCOPE -- A tree that holds one sub-tree and a scope.
enum class tree_arity_v
  {
    NULLARY,
    UNARY,
    BINARY,
    TERNARY,
    STR,
    CST,
    SCOPE,
    BUILTIN,
  };

// CAT -- A node for holding concatenation (X Y Z).
// ALT -- A node for holding alternation (X, Y, Z).
// CAPTURE -- For holding [X].
// OR -- For holding first-match alternation (X || Y || Z)
//
// NOP -- For holding a no-op that comes up in "%s" and (,X).
//
// CLOSE_STAR -- For holding X*.  X is the only child.
// CLOSE_PLUS -- For holding X+; X? is emulated as (X,).
//
// ASSERT -- All assertions (such as ?some_tag) are modeled using an
// ASSERT node, whose only child is a predicate node expressing the
// asserted condition (such as PRED_TAG).  Negative assertions are
// modeled using PRED_NOT.  In particular, IF is expanded into (!empty
// drop), ELSE into (?empty drop).
//
// PRED_SUBX_ANY -- For holding ?(X).  !() is modeled using PRED_NOT.
//
// PRED_SUBX_CMP -- For holding X == Y.  The third operand is the
// operation predicate.  (E.g. F_BUILTIN<EQ>).
//
// CONST -- For holding named constants and integer literals.
//
// STR -- For holding string literals.  Actual string literals are
// translated to FORMAT.
//
// FORMAT -- The literal parts of the format string are stored as
// children of type STR.  Other computation is stored as children of
// other types.

#define TREE_TYPES				\
  TREE_TYPE (CAT, BINARY)			\
  TREE_TYPE (ALT, BINARY)			\
  TREE_TYPE (OR, BINARY)			\
  TREE_TYPE (CAPTURE, UNARY)			\
  TREE_TYPE (SUBX_EVAL, CST)			\
  TREE_TYPE (IFELSE, TERNARY)			\
  TREE_TYPE (SCOPE, SCOPE)			\
  TREE_TYPE (BLOCK, UNARY)			\
  TREE_TYPE (BIND, STR)				\
  TREE_TYPE (READ, STR)				\
  TREE_TYPE (NOP, NULLARY)			\
  TREE_TYPE (CLOSE_STAR, UNARY)			\
  TREE_TYPE (CLOSE_PLUS, UNARY)			\
  TREE_TYPE (ASSERT, UNARY)			\
  TREE_TYPE (EMPTY_LIST, NULLARY)		\
  TREE_TYPE (PRED_AND, BINARY)			\
  TREE_TYPE (PRED_OR, BINARY)			\
  TREE_TYPE (PRED_NOT, UNARY)			\
  TREE_TYPE (PRED_SUBX_ANY, UNARY)		\
  TREE_TYPE (PRED_SUBX_CMP, TERNARY)		\
  TREE_TYPE (CONST, CST)			\
  TREE_TYPE (STR, STR)				\
  TREE_TYPE (FORMAT, NULLARY)			\
  TREE_TYPE (F_DEBUG, NULLARY)			\
  TREE_TYPE (F_BUILTIN, BUILTIN)

enum class tree_type
  {
#define TREE_TYPE(ENUM, ARITY) ENUM,
    TREE_TYPES
#undef TREE_TYPE
  };

template <tree_type TT> class tree_arity;
#define TREE_TYPE(ENUM, ARITY)					\
  template <> struct tree_arity <tree_type::ENUM> {		\
    static const tree_arity_v value = tree_arity_v::ARITY;	\
  };
TREE_TYPES
#undef TREE_TYPE

class op;
class pred;
class scope;

// This is for communication between lexical and syntactic analyzers
// and the rest of the world.  It uses naked pointers all over the
// place, as in the %union that the lexer and parser use, we can't
// hold smart pointers.
struct tree
{
  std::unique_ptr <std::string> m_str;
  std::unique_ptr <constant> m_cst;
  std::shared_ptr <builtin const> m_builtin;

public:
  tree_type m_tt;
  std::vector <tree> m_children;

  tree ();
  explicit tree (tree_type tt);
  tree (tree const &other);

  tree (tree_type tt, std::string const &str);
  tree (tree_type tt, constant const &cst);

  tree &operator= (tree other);
  void swap (tree &other);

  tree &child (size_t idx);
  tree const &child (size_t idx) const;
  tree_type tt () const { return m_tt; }

  std::string &str () const;
  constant &cst () const;

  void push_child (tree const &t);

  bool operator< (tree const &that) const;

  // === Build interface ===
  //
  // The following methods are implemented in build.cc.  They are for
  // translation of trees to actual execution nodes.

  // Remove unnecessary operations--some stack shuffling can be
  // eliminated and protect nodes.
  // XXX this should actually be hidden behind build_exec or what not.
  void simplify ();

  // This should build an op node corresponding to this expression.
  //
  // Not every expression node needs to have an associated op, some
  // nodes will only install some plumbing (in particular, a CAT node
  // would only create a series of nested op's).  UPSTREAM should be
  // an op_origin if this is the toplevel-most expression, otherwise it
  // should be a valid op that the op produced by this node feeds off.
  std::shared_ptr <op>
  build_exec (layout &l, std::shared_ptr <op> upstream) const;

  // === Parser interface ===
  //
  // The following methods are implemented in tree_cr.hh and
  // tree_cr.cc.  They are meant as parser interface, otherwise value
  // semantics should be preferred.

  template <tree_type TT> static std::unique_ptr <tree>
  create_nullary ();

  template <tree_type TT> static std::unique_ptr <tree>
  create_unary (std::unique_ptr <tree> op);

  template <tree_type TT> static std::unique_ptr <tree>
  create_binary (std::unique_ptr <tree> lhs, std::unique_ptr <tree> rhs);

  template <tree_type TT> static std::unique_ptr <tree>
  create_ternary (std::unique_ptr <tree> op1, std::unique_ptr <tree> op2,
		  std::unique_ptr <tree> op3);

  template <tree_type TT> static std::unique_ptr <tree>
  create_str (std::string s);

  template <tree_type TT> static std::unique_ptr <tree>
  create_const (constant c);

  template <tree_type TT> static std::unique_ptr <tree>
  create_cat (std::unique_ptr <tree> t1, std::unique_ptr <tree> t2);

  static std::unique_ptr <tree>
  create_builtin (std::shared_ptr <builtin const> b);

  static std::unique_ptr <tree>
  create_neg (std::unique_ptr <tree> t1);

  static std::unique_ptr <tree>
  create_assert (std::unique_ptr <tree> t1);

  static std::unique_ptr <tree>
  create_scope (std::unique_ptr <tree> t1);

  static tree resolve_scopes (tree t);

  // push_back (*T) and delete T.
  void take_child (std::unique_ptr <tree> t);

  // push_front (*T) and delete T.
  void take_child_front (std::unique_ptr <tree> t);

  // Takes a tree T, which is a CAT or an ALT, and appends all
  // children therein.  It then deletes T.
  void take_cat (std::unique_ptr <tree> t);

  friend std::ostream &operator<< (std::ostream &o, tree const &t);
};

std::ostream &operator<< (std::ostream &o, tree const &t);

#endif /* _TREE_H_ */
