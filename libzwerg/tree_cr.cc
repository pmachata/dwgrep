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

#include "scope.hh"
#include "tree_cr.hh"

tree *
tree::create_builtin (std::shared_ptr <builtin const> b)
{
  auto t = new tree {tree_type::F_BUILTIN};
  t->m_builtin = b;
  return t;
}

tree *
tree::create_neg (tree *t)
{
  return tree::create_unary <tree_type::PRED_NOT> (t);
}

tree *
tree::create_assert (tree *t)
{
  return tree::create_unary <tree_type::ASSERT> (t);
}

tree *
tree::create_scope (tree *t1)
{
  auto t = new tree {tree_type::SCOPE, std::make_shared <scope> (nullptr)};
  t->take_child (t1);
  return t;
}

void
tree::take_child (tree *t)
{
  m_children.push_back (*t);
  delete t;
}

void
tree::take_child_front (tree *t)
{
  m_children.insert (m_children.begin (), *t);
  delete t;
}

void
tree::take_cat (tree *t)
{
  m_children.insert (m_children.end (),
		     t->m_children.begin (), t->m_children.end ());
  delete t;
}

namespace
{
  tree
  promote_scopes (tree t, std::shared_ptr <scope> scp)
  {
    switch (t.tt ())
      {
      case tree_type::BIND:
	if (scp->has_name (t.str ()))
	  throw std::runtime_error (std::string ("Name `")
				    + t.str () + "' rebound.");

	scp->add_name (t.str ());
	assert (t.m_children.size () == 0);
	return t;

      case tree_type::SCOPE:
	// First descend, so as to process all binds.
	assert (t.m_children.size () == 1);
	t.child (0) = promote_scopes (t.child (0), t.scp ());

	// Only keep the scope if it is useful.
	if (t.scp ()->empty ())
	  return t.child (0);
	else
	  return t;

      default:
	for (auto &c: t.m_children)
	  c = promote_scopes (c, scp);
	return t;
      }
  }

  void
  link_scopes (tree &t, std::shared_ptr <scope> scp)
  {
    switch (t.tt ())
      {
      case tree_type::SCOPE:
	t.scp ()->parent = scp;
	link_scopes (t.child (0), t.scp ());
	break;

      case tree_type::BIND:
      case tree_type::READ:
	{
	  assert (t.m_scope == nullptr);
	  assert (t.m_cst == nullptr);

	  // Find access coordinates.  Stack frames form a chain.  Here
	  // we find what stack frame will be accessed (i.e. how deeply
	  // nested is this OP inside SCOPE's) and at which position.

	  size_t depth = 0;
	  for (; scp != nullptr; scp = scp->parent, ++depth)
	    if (scp->has_name (t.str ()))
	      {
		t.m_scope = scp;
		t.m_cst = std::make_unique <constant> (depth, nullptr);
		return;
	      }

	  throw std::runtime_error (std::string ("Unknown identifier `")
				    + t.str () + "'.");
	}

      default:
	for (auto &c: t.m_children)
	  link_scopes (c, scp);
	break;
      }
  }
}

// Walk the tree, and collect all binds to the nearest higher scope.
// Keep only those scopes that actually contain any variables.
tree
tree::resolve_scopes (tree t)
{
  // Create whole-program scope.
  tree u {tree_type::SCOPE, std::make_shared <scope> (nullptr)};
  u.push_child (t);
  tree ret = ::promote_scopes (u, nullptr);
  link_scopes (ret, nullptr);
  return ret;
}
