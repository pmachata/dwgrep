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
#include "scope.hh"
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
          auto origin = std::make_shared <op_origin> (nullptr);
          auto op = build_exec (t.child (0), l, rdv_ll, origin, bn, up);
          return std::make_unique <pred_subx_any> (op, origin);
        }

      case tree_type::PRED_SUBX_CMP:
        {
          assert (t.m_children.size () == 3);
          auto origin = std::make_shared <op_origin> (nullptr);
          auto op1 = build_exec (t.child (0), l, rdv_ll, origin, bn, up);
          auto op2 = build_exec (t.child (1), l, rdv_ll, origin, bn, up);
          auto pred = build_pred (t.child (2), l, rdv_ll, bn, up);
          return std::make_unique <pred_subx_compare> (op1, op2, origin,
                                                       std::move (pred));
        }

      case tree_type::F_BUILTIN:
        return t.m_builtin->build_pred ();

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
  build_exec (tree const &t, layout &l, layout::loc rdv_ll,
	      std::shared_ptr <op> upstream,
	      bindings &bn, uprefs &up)
  {
    if (upstream == nullptr)
      upstream = std::make_shared <op_origin> (std::make_unique <stack> ());

    switch (t.m_tt)
      {
      case tree_type::CAT:
        for (auto const &ch: t.m_children)
          upstream = build_exec (ch, l, rdv_ll, upstream, bn, up);
        return upstream;

      case tree_type::ALT:
        {
          auto done = std::make_shared <bool> (false);

          op_merge::opvec_t ops;
          {
            auto f = std::make_shared <std::vector <stack::uptr>>
              (t.m_children.size ());
            for (size_t i = 0; i < t.m_children.size (); ++i)
              ops.push_back (std::make_shared <op_tine> (upstream, f, done, i));
          }

          auto build_branch = [&] (tree const &ch, std::shared_ptr <op> o)
            {
              return build_exec (ch, l, rdv_ll, o, bn, up);
            };

          std::transform (t.m_children.begin (), t.m_children.end (),
                          ops.begin (), ops.begin (), build_branch);

          return std::make_shared <op_merge> (ops, done);
        }

      case tree_type::OR:
        {
          auto o = std::make_shared <op_or> (upstream);
          for (auto const &ch: t.m_children)
            {
              auto origin2 = std::make_shared <op_origin> (nullptr);
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
          auto op = t.m_builtin->build_exec (upstream);
          assert (op != nullptr);
          return op;
        }

      case tree_type::ASSERT:
        return std::make_shared <op_assert>
          (upstream, build_pred (t.child (0), l, rdv_ll, bn, up));

      case tree_type::FORMAT:
        {
          auto s_origin = std::make_shared <stringer_origin> ();
          std::shared_ptr <stringer> strgr = s_origin;
          for (auto it = t.m_children.rbegin (), eit = t.m_children.rend ();
               it != eit; ++it)
            {
              auto const &tree = *it;
              if (tree.m_tt == tree_type::STR)
                strgr = std::make_shared <stringer_lit> (strgr, tree.str ());
              else
                {
                  auto origin2 = std::make_shared <op_origin> (nullptr);
                  auto op = build_exec (tree, l, rdv_ll, origin2, bn, up);
                  strgr = std::make_shared <stringer_op> (strgr, origin2, op);
                }
            }

          return std::make_shared <op_format> (upstream, s_origin, strgr);
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
          auto origin = std::make_shared <op_origin> (nullptr);
          auto op = build_exec (t.child (0), l, rdv_ll, origin, bn, up);
          return std::make_shared <op_capture> (upstream, origin, op);
        }

      case tree_type::SUBX_EVAL:
        {
          auto origin = std::make_shared <op_origin> (nullptr);
          auto op = build_exec (t.child (0), l, rdv_ll, origin, bn, up);
          return std::make_shared <op_subx> (upstream, origin, op,
                                             t.cst ().value ().uval ());
        }

      case tree_type::CLOSE_STAR:
        {
          auto origin = std::make_shared <op_origin> (nullptr);
          auto op = build_exec (t.child (0), l, rdv_ll, origin, bn, up);
          return std::make_shared <op_tr_closure> (upstream, origin, op,
                                                   op_tr_closure_kind::star);
        }

      case tree_type::CLOSE_PLUS:
        {
          auto origin = std::make_shared <op_origin> (nullptr);
          auto op = build_exec (t.child (0), l, rdv_ll, origin, bn, up);
          return std::make_shared <op_tr_closure> (upstream, origin, op,
                                                   op_tr_closure_kind::plus);
        }

      case tree_type::SCOPE:
        {
          bindings scope {bn};
          auto origin = std::make_shared <op_origin> (nullptr);
          auto op = build_exec (t.child (0), l, rdv_ll, origin, scope, up);
          return std::make_shared <op_scope> (upstream, origin, op,
                                              t.scp ()->num_names ());
        }

      case tree_type::BLOCK:
        assert (t.scp () == nullptr);
        return std::make_shared <op_lex_closure> (upstream, t.child (0));

      case tree_type::BIND:
        {
          auto ret = std::make_shared <op_bind> (upstream,
                                                 t.cst ().value ().uval (),
                                                 t.scp ()->index (t.str ()));
          bn.bind (t.str (), *ret);
          return ret;
        }

      case tree_type::READ:
	{
	  auto op = std::make_shared <op_read> (upstream,
						t.cst ().value ().uval (),
						t.scp ()->index (t.str ()));
	  return std::make_shared <op_apply> (op, true);
	}

      case tree_type::F_DEBUG:
        return std::make_shared <op_f_debug> (upstream);

      case tree_type::IFELSE:
        {
          auto cond_origin = std::make_shared <op_origin> (nullptr);
          auto cond_op = build_exec (t.child (0), l, rdv_ll, cond_origin, bn, up);

          auto then_origin = std::make_shared <op_origin> (nullptr);
          auto then_op = build_exec (t.child (1), l, rdv_ll, then_origin, bn, up);

          auto else_origin = std::make_shared <op_origin> (nullptr);
          auto else_op = build_exec (t.child (2), l, rdv_ll, else_origin, bn, up);

          return std::make_shared <op_ifelse> (upstream, cond_origin, cond_op,
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
tree::build_exec (layout &l, std::shared_ptr <op> upstream) const
{
  uprefs up;
  bindings bn;
  layout::loc no_ll {0xdeadbeef};
  return ::build_exec (*this, l, no_ll, upstream, bn, up);
}
