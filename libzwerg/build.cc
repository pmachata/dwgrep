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

#include <algorithm>
#include <iostream>
#include <memory>

#include "op.hh"
#include "tree.hh"
#include "value-cst.hh"
#include "value-seq.hh"
#include "value-str.hh"
#include "builtin-closure.hh"
#include "bindings.hh"

namespace
{
  std::shared_ptr <op>
  build_exec (tree const &t, layout &l, layout::loc rdv_ll,
	      std::shared_ptr <op> upstream,
	      bindings &bn, uprefs &up);

  std::unique_ptr <pred>
  build_pred (tree const &t, layout &l, layout::loc rdv_ll,
	      bindings &bn, uprefs &up)
  {
    switch (t.m_tt)
      {
      case tree_type::PRED_NOT:
	return std::make_unique <pred_not> (build_pred (t.child (0), l, rdv_ll, bn, up));

      case tree_type::PRED_OR:
	return std::make_unique <pred_or>
	  (build_pred (t.m_children[0], l, rdv_ll, bn, up),
	   build_pred (t.m_children[1], l, rdv_ll, bn, up));

      case tree_type::PRED_AND:
	return std::make_unique <pred_and>
	  (build_pred (t.m_children[0], l, rdv_ll, bn, up),
	   build_pred (t.m_children[1], l, rdv_ll, bn, up));

      case tree_type::PRED_SUBX_ANY:
	{
	  assert (t.m_children.size () == 1);
	  auto origin = std::make_shared <op_origin> (l);
	  auto op = build_exec (t.child (0), l, rdv_ll, origin, bn, up);
	  return std::make_unique <pred_subx_any> (op, origin);
	}

      case tree_type::PRED_SUBX_CMP:
	{
	  assert (t.m_children.size () == 3);
	  auto origin = std::make_shared <op_origin> (l);
	  auto op1 = build_exec (t.child (0), l, rdv_ll, origin, bn, up);
	  auto op2 = build_exec (t.child (1), l, rdv_ll, origin, bn, up);
	  auto pred = build_pred (t.child (2), l, rdv_ll, bn, up);
	  return std::make_unique <pred_subx_compare> (op1, op2, origin,
						       std::move (pred));
	}

      case tree_type::F_BUILTIN:
	return t.m_builtin->build_pred (l);

      case tree_type::CAT:
      case tree_type::NOP:
      case tree_type::ASSERT:
      case tree_type::ALT:
      case tree_type::OR:
      case tree_type::CAPTURE:
      case tree_type::SUBX_EVAL:
      case tree_type::EMPTY_LIST:
      case tree_type::CLOSE_STAR:
      case tree_type::CLOSE_PLUS:
      case tree_type::CONST:
      case tree_type::STR:
      case tree_type::FORMAT:
      case tree_type::F_DEBUG:
      case tree_type::BIND:
      case tree_type::READ:
      case tree_type::SCOPE:
      case tree_type::BLOCK:
      case tree_type::IFELSE:
	assert (! "Should never get here.");
	abort ();
      }
    abort ();
  }

  std::shared_ptr <op>
  build_builtin (builtin const &bi,
		 std::shared_ptr <op> upstream,
		 layout &l)
  {
    if (auto pred = bi.build_pred (l))
      return std::make_shared <op_assert> (upstream, std::move (pred));

    auto op = bi.build_exec (l, upstream);
    assert (op != nullptr);
    return op;
  }

  std::shared_ptr <op>
  build_exec (tree const &t, layout &l, layout::loc rdv_ll,
	      std::shared_ptr <op> upstream,
	      bindings &bn, uprefs &up)
  {
    switch (t.m_tt)
      {
      case tree_type::CAT:
	for (auto const &ch: t.m_children)
	  upstream = build_exec (ch, l, rdv_ll, upstream, bn, up);
	return upstream;

      case tree_type::ALT:
	{
	  auto merge = std::make_shared <op_merge> (l, upstream);

	  for (size_t i = 0; i < t.m_children.size (); ++i)
	    {
	      auto tine = std::make_shared <op_tine> (*merge, i);
	      auto op = build_exec (t.m_children[i], l, rdv_ll, tine, bn, up);
	      merge->add_branch (op);
	    }

	  return merge;
	}

      case tree_type::OR:
	{
	  auto o = std::make_shared <op_or> (l, upstream);
	  for (auto const &ch: t.m_children)
	    {
	      auto origin2 = std::make_shared <op_origin> (l);
	      auto op = build_exec (ch, l, rdv_ll, origin2, bn, up);
	      o->add_branch (origin2, op);
	    }
	  return o;
	}

      case tree_type::NOP:
	return std::make_shared <op_nop> (upstream);

      case tree_type::F_BUILTIN:
	{
	  if (auto pred = build_pred (t, l, rdv_ll, bn, up))
	    return std::make_shared <op_assert> (upstream, std::move (pred));
	  auto op = t.m_builtin->build_exec (l, upstream);
	  assert (op != nullptr);
	  return op;
	}

      case tree_type::ASSERT:
	return std::make_shared <op_assert>
	  (upstream, build_pred (t.child (0), l, rdv_ll, bn, up));

      case tree_type::FORMAT:
	{
	  auto s_origin = std::make_shared <stringer_origin> (l);
	  std::shared_ptr <stringer> strgr = s_origin;
	  for (auto it = t.m_children.rbegin (), eit = t.m_children.rend ();
	       it != eit; ++it)
	    {
	      auto const &tree = *it;
	      if (tree.m_tt == tree_type::STR)
		strgr = std::make_shared <stringer_lit> (strgr, tree.str ());
	      else
		{
		  auto origin2 = std::make_shared <op_origin> (l);
		  auto op = build_exec (tree, l, rdv_ll, origin2, bn, up);
		  strgr = std::make_shared <stringer_op> (l, strgr,
							  origin2, op);
		}
	    }

	  return std::make_shared <op_format> (l, upstream, s_origin, strgr);
	}

      case tree_type::CONST:
	{
	  auto val = std::make_unique <value_cst> (t.cst (), 0);
	  return std::make_shared <op_const> (upstream, std::move (val));
	}

      case tree_type::STR:
	{
	  auto val = std::make_unique <value_str> (std::string (t.str ()), 0);
	  return std::make_shared <op_const> (upstream, std::move (val));
	}

      case tree_type::EMPTY_LIST:
	{
	  auto val = std::make_unique <value_seq> (value_seq::seq_t {}, 0);
	  return std::make_shared <op_const> (upstream, std::move (val));
	}

      case tree_type::CAPTURE:
	{
	  auto origin = std::make_shared <op_origin> (l);
	  auto op = build_exec (t.child (0), l, rdv_ll, origin, bn, up);
	  return std::make_shared <op_capture> (upstream, origin, op);
	}

      case tree_type::SUBX_EVAL:
	{
	  auto origin = std::make_shared <op_origin> (l);
	  auto op = build_exec (t.child (0), l, rdv_ll, origin, bn, up);
	  return std::make_shared <op_subx> (l, upstream, origin, op,
					     t.cst ().value ().uval ());
	}

      case tree_type::CLOSE_STAR:
	{
	  auto origin = std::make_shared <op_origin> (l);
	  auto op = build_exec (t.child (0), l, rdv_ll, origin, bn, up);
	  return std::make_shared <op_tr_closure> (l, upstream, origin, op,
						   op_tr_closure_kind::star);
	}

      case tree_type::CLOSE_PLUS:
	{
	  auto origin = std::make_shared <op_origin> (l);
	  auto op = build_exec (t.child (0), l, rdv_ll, origin, bn, up);
	  return std::make_shared <op_tr_closure> (l, upstream, origin, op,
						   op_tr_closure_kind::plus);
	}

      case tree_type::SCOPE:
	{
	  bindings scope {bn};
	  return build_exec (t.child (0), l, rdv_ll, upstream, scope, up);
	}

      case tree_type::BLOCK:
	{
	  uprefs inner_up {bn, up};
	  bindings inner_bn;
	  layout inner_l;
	  layout::loc inner_rdv_ll = op_apply::reserve_rendezvous (inner_l);
	  auto origin = std::make_shared <op_origin> (inner_l);
	  auto op = build_exec (t.child (0), inner_l, inner_rdv_ll, origin,
				inner_bn, inner_up);

	  std::map <unsigned, std::string> refd_ids = inner_up.refd_ids ();
	  // Walk the refd names in backward order of their ID, so that
	  // op_lex_closure can pop the upvalues from the stack in natural
	  // order.
	  for (auto it = refd_ids.rbegin (); it != refd_ids.rend (); ++it)
	    if (const binding *b = bn.find (it->second))
	      {
		assert (! b->is_builtin ());
		upstream = std::make_shared <op_read> (upstream,
						       b->get_bind ());
	      }
	    else
	      {
		upref *upr = up.find (it->second);
		assert (upr != nullptr);
		assert (! upr->is_builtin ());
		upstream = std::make_shared <op_upread> (upstream,
							 upr->get_id (),
							 rdv_ll);
	      }

	  return std::make_shared <op_lex_closure> (upstream, inner_l,
						    inner_rdv_ll, origin, op,
						    refd_ids.size ());
	}

      case tree_type::BIND:
	{
	  auto ret = std::make_shared <op_bind> (l, upstream);
	  bn.bind (t.str (), *ret);
	  return ret;
	}

      case tree_type::READ:
	{
	  if (const binding *b = bn.find (t.str ()))
	    {
	      if (b->is_builtin ())
		return build_builtin (b->get_builtin (), upstream, l);

	      auto op = std::make_shared <op_read> (upstream, b->get_bind ());
	      return std::make_shared <op_apply> (l, op, true);
	    }

	  if (upref *upr = up.find (t.str ()))
	    {
	      if (upr->is_builtin ())
		return build_builtin (upr->get_builtin (), upstream, l);

	      auto op = std::make_shared <op_upread> (upstream, upr->get_id (),
						      rdv_ll);
	      return std::make_shared <op_apply> (l, op, true);
	    }

	  throw std::runtime_error
	    (std::string () + "Attempt to read an unbound name `"
	     + t.str () + "'");
	}

      case tree_type::F_DEBUG:
	return std::make_shared <op_f_debug> (upstream);

      case tree_type::IFELSE:
	{
	  auto cond_subl = l;
	  auto cond_origin = std::make_shared <op_origin> (cond_subl);
	  auto cond_op = build_exec (t.child (0), cond_subl, rdv_ll,
				     cond_origin, bn, up);

	  auto then_subl = l;
	  auto then_origin = std::make_shared <op_origin> (then_subl);
	  auto then_op = build_exec (t.child (1), then_subl, rdv_ll,
				     then_origin, bn, up);

	  auto else_subl = l;
	  auto else_origin = std::make_shared <op_origin> (else_subl);
	  auto else_op = build_exec (t.child (2), else_subl, rdv_ll,
				     else_origin, bn, up);

	  l.add_union ({cond_subl, then_subl, else_subl});
	  return std::make_shared <op_ifelse> (l, upstream,
					       cond_origin, cond_op,
					       then_origin, then_op,
					       else_origin, else_op);
	}

      case tree_type::PRED_AND:
      case tree_type::PRED_OR:
      case tree_type::PRED_NOT:
      case tree_type::PRED_SUBX_ANY:
      case tree_type::PRED_SUBX_CMP:
	assert (! "Should never get here.");
	abort ();
      }

    abort ();
  }
}

std::shared_ptr <op>
tree::build_exec (layout &l, std::shared_ptr <op> upstream,
		  vocabulary &voc) const
{
  uprefs up;
  bindings root {voc};
  bindings bn {root};
  layout::loc no_ll {0xdeadbeef};
  return ::build_exec (*this, l, no_ll, upstream, bn, up);
}
