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

#include <algorithm>
#include <cassert>
#include <climits>
#include <iostream>
#include <set>
#include <stdexcept>

#include "tree.hh"

namespace
{
  const tree_arity_v argtype[] = {
#define TREE_TYPE(ENUM, ARITY) tree_arity_v::ARITY,
    TREE_TYPES
#undef TREE_TYPE
  };
}

tree::tree ()
  : tree (static_cast <tree_type> (-1))
{}

tree::tree (tree_type tt)
  : m_builtin {nullptr}
  , m_tt {tt}
{}

namespace
{
  template <class T>
  std::unique_ptr <T>
  copy_unique (std::unique_ptr <T> const &ptr)
  {
    if (ptr == nullptr)
      return nullptr;
    else
      return std::make_unique <T> (*ptr);
  }
}

tree::tree (tree const &other)
  : m_str {copy_unique (other.m_str)}
  , m_cst {copy_unique (other.m_cst)}
  , m_builtin {other.m_builtin}
  , m_tt {other.m_tt}
  , m_children {other.m_children}
{}

tree::tree (tree_type tt, std::string const &str)
  : m_str {std::make_unique <std::string> (str)}
  , m_builtin {nullptr}
  , m_tt {tt}
{}

tree::tree (tree_type tt, constant const &cst)
  : m_cst {std::make_unique <constant> (cst)}
  , m_builtin {nullptr}
  , m_tt {tt}
{}

tree &
tree::operator= (tree other)
{
  this->swap (other);
  return *this;
}

void
tree::swap (tree &other)
{
  std::swap (m_str, other.m_str);
  std::swap (m_cst, other.m_cst);
  std::swap (m_builtin, other.m_builtin);
  std::swap (m_tt, other.m_tt);
  std::swap (m_children, other.m_children);
}

tree &
tree::child (size_t idx)
{
  assert (idx < m_children.size ());
  return m_children[idx];
}

tree const &
tree::child (size_t idx) const
{
  assert (idx < m_children.size ());
  return m_children[idx];
}

std::string &
tree::str () const
{
  assert (m_str != nullptr);
  return *m_str;
}

constant &
tree::cst () const
{
  assert (m_cst != nullptr);
  return *m_cst;
}

void
tree::push_child (tree const &t)
{
  m_children.push_back (t);
}

std::ostream &
operator<< (std::ostream &o, tree const &t)
{
  o << "(";

  switch (t.m_tt)
    {
#define TREE_TYPE(ENUM, ARITY) case tree_type::ENUM: o << #ENUM; break;
      TREE_TYPES
#undef TREE_TYPE
    }

  switch (argtype[(int) t.m_tt])
    {
    case tree_arity_v::CST:
      o << "<" << t.cst () << ">";
      break;

    case tree_arity_v::STR:
      o << "<" << t.str () << ">";
      break;

    case tree_arity_v::SCOPE:
    case tree_arity_v::NULLARY:
    case tree_arity_v::UNARY:
    case tree_arity_v::BINARY:
    case tree_arity_v::TERNARY:
      break;

    case tree_arity_v::BUILTIN:
      o << "<" << t.m_builtin->name () << ">";
      break;
    }

  for (auto const &child: t.m_children)
    o << " " << child;

  return o << ")";
}

bool
tree::operator< (tree const &that) const
{
  if (m_children.size () < that.m_children.size ())
    return true;
  else if (m_children.size () > that.m_children.size ())
    return false;

  auto it1 = m_children.begin ();
  auto it2 = that.m_children.begin ();
  for (; it1 != m_children.end (); ++it1, ++it2)
    if (*it1 < *it2)
      return true;
    else if (*it2 < *it1)
      return false;

  switch (argtype[(int) m_tt])
    {
    case tree_arity_v::CST:
      // Assume both are set.
      assert (m_cst != nullptr);
      assert (that.m_cst != nullptr);
      return *m_cst < *that.m_cst;

    case tree_arity_v::STR:
      // Assume both are set.
      assert (m_str != nullptr);
      assert (that.m_str != nullptr);
      return *m_str < *that.m_str;

    case tree_arity_v::SCOPE:
      return child (0) < that.child (0);

    case tree_arity_v::BUILTIN:
      return m_builtin < that.m_builtin;

    case tree_arity_v::NULLARY:
    case tree_arity_v::UNARY:
    case tree_arity_v::BINARY:
    case tree_arity_v::TERNARY:
      return false;
    }

  assert (! "Should never be reached.");
  abort ();
}

void
tree::simplify ()
{
  // Recurse.
  for (auto &t: m_children)
    t.simplify ();

  // Promote CAT's in CAT nodes and ALT's in ALT nodes.  Parser does
  // this already, but other transformations may lead to re-emergence
  // of this pattern.
  if (m_tt == tree_type::CAT || m_tt == tree_type::ALT)
    while (true)
      {
	bool changed = false;
	for (size_t i = 0; i < m_children.size (); )
	  if (child (i).m_tt == m_tt)
	    {
	      std::vector <tree> nchildren = m_children;

	      tree tmp = std::move (nchildren[i]);
	      nchildren.erase (nchildren.begin () + i);
	      nchildren.insert (nchildren.begin () + i,
				tmp.m_children.begin (),
				tmp.m_children.end ());

	      m_children = std::move (nchildren);
	      changed = true;
	    }
	  else
	    ++i;

	if (! changed)
	  break;
      }

  // Promote CAT's only child.
  if (m_tt == tree_type::CAT && m_children.size () == 1)
    {
      *this = child (0);
      simplify ();
    }

  // Change (FORMAT (STR)) to (STR).
  if (m_tt == tree_type::FORMAT
      && m_children.size () == 1
      && child (0).m_tt == tree_type::STR)
    {
      *this = child (0);
      simplify ();
    }

  if (m_tt == tree_type::CAT)
    {
      // Drop NOP's in CAT nodes.
      auto it = std::remove_if (m_children.begin (), m_children.end (),
				[] (tree &t) {
				  return t.m_tt == tree_type::NOP;
				});
      if (it != m_children.end ())
	{
	  m_children.erase (it, m_children.end ());
	  simplify ();
	}
    }
}
