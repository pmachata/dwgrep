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

#include "tree_cr.hh"

std::unique_ptr <tree>
tree::create_builtin (std::shared_ptr <builtin const> b)
{
  auto t = std::make_unique <tree> (tree_type::F_BUILTIN);
  t->m_builtin = b;
  return t;
}

std::unique_ptr <tree>
tree::create_neg (std::unique_ptr <tree> t)
{
  return tree::create_unary <tree_type::PRED_NOT> (std::move (t));
}

std::unique_ptr <tree>
tree::create_assert (std::unique_ptr <tree> t)
{
  return tree::create_unary <tree_type::ASSERT> (std::move (t));
}

std::unique_ptr <tree>
tree::create_scope (std::unique_ptr <tree> t1)
{
  auto t = std::make_unique <tree> (tree_type::SCOPE);
  t->take_child (std::move (t1));
  return t;
}

void
tree::take_child (std::unique_ptr <tree> t)
{
  m_children.push_back (*t);
}

void
tree::take_child_front (std::unique_ptr <tree> t)
{
  m_children.insert (m_children.begin (), *t);
}

void
tree::take_cat (std::unique_ptr <tree> t)
{
  m_children.insert (m_children.end (),
		     t->m_children.begin (), t->m_children.end ());
}

namespace
{
  // Shallow sweep through all children, but stopping at scopes, to determine if
  // this particular scope has any bindings.
  bool
  scope_has_bindings (tree t)
  {
    switch (t.tt ())
      {
      case tree_type::SCOPE:
	return false;
      case tree_type::BIND:
	return true;
      default:
	for (auto const &ch: t.m_children)
	  if (scope_has_bindings (ch))
	    return true;
	return false;
      }
  }

  tree
  promote_scopes (tree t)
  {
    switch (t.tt ())
      {
      case tree_type::SCOPE:
	assert (t.m_children.size () == 1);
	t.child (0) = promote_scopes (t.child (0));

	// Only keep the scope if it is useful.
	if (! scope_has_bindings(t.child (0)))
	  return t.child (0);
	else
	  return t;

      default:
	for (auto &c: t.m_children)
	  c = promote_scopes (c);
	return t;
      }
  }
}

// Walk the tree, and collect all binds to the nearest higher scope.
// Keep only those scopes that actually contain any variables.
tree
tree::resolve_scopes (tree t)
{
  // Create whole-program scope.
  tree u {tree_type::SCOPE};
  u.push_child (t);
  tree ret = ::promote_scopes (u);
  return ret;
}
