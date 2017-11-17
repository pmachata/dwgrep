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
#include "bindings.hh"

namespace
{
  std::shared_ptr <op>
  build_exec (tree const &t, std::shared_ptr <op> upstream, bindings &bn);

  std::unique_ptr <pred>
  build_pred (tree const &t, bindings &bn)
  {
    switch (t.m_tt)
      {
      case tree_type::PRED_NOT:
        return std::make_unique <pred_not> (build_pred (t.child (0), bn));

      case tree_type::PRED_OR:
        return std::make_unique <pred_or>
          (build_pred (t.m_children[0], bn),
           build_pred (t.m_children[1], bn));

      case tree_type::PRED_AND:
        return std::make_unique <pred_and>
          (build_pred (t.m_children[0], bn),
           build_pred (t.m_children[1], bn));

      case tree_type::PRED_SUBX_ANY:
        {
          assert (t.m_children.size () == 1);
          auto origin = std::make_shared <op_origin> ();
          auto op = build_exec (t.child (0), origin, bn);
          return std::make_unique <pred_subx_any> (op, origin);
        }

      case tree_type::PRED_SUBX_CMP:
        {
          assert (t.m_children.size () == 3);
          auto origin = std::make_shared <op_origin> ();
          auto op1 = build_exec (t.child (0), origin, bn);
          auto op2 = build_exec (t.child (1), origin, bn);
          auto pred = build_pred (t.child (2), bn);
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
  build_exec (tree const &t, std::shared_ptr <op> upstream, bindings &bn)
  {
    assert (upstream != nullptr);

    switch (t.m_tt)
      {
      case tree_type::CAT:
        for (auto const &ch: t.m_children)
          upstream = build_exec (ch, upstream, bn);
        return upstream;

      case tree_type::ALT:
        {
	  auto merge = std::make_shared <op_merge> (upstream);

	  for (size_t i = 0; i < t.m_children.size (); ++i)
	    {
	      auto tine = std::make_shared <op_tine> (*merge, i);
              auto op = build_exec (t.m_children[i], tine, bn);
	      merge->add_branch (op);
	    }

          return merge;
        }

      case tree_type::OR:
        {
          auto o = std::make_shared <op_or> (upstream);
          for (auto const &ch: t.m_children)
            {
              auto origin2 = std::make_shared <op_origin> ();
              auto op = build_exec (ch, origin2, bn);
              o->add_branch (origin2, op);
            }
          return o;
        }

      case tree_type::NOP:
        return std::make_shared <op_nop> (upstream);

      case tree_type::F_BUILTIN:
        {
          if (auto pred = build_pred (t, bn))
            return std::make_shared <op_assert> (upstream, std::move (pred));
          auto op = t.m_builtin->build_exec (upstream);
          assert (op != nullptr);
          return op;
        }

      case tree_type::ASSERT:
        return std::make_shared <op_assert>
          (upstream, build_pred (t.child (0), bn));

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
                  auto origin2 = std::make_shared <op_origin> ();
                  auto op = build_exec (tree, origin2, bn);
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
          auto origin = std::make_shared <op_origin> ();
          auto op = build_exec (t.child (0), origin, bn);
          return std::make_shared <op_capture> (upstream, origin, op);
        }

      case tree_type::SUBX_EVAL:
        {
          auto origin = std::make_shared <op_origin> ();
          auto op = build_exec (t.child (0), origin, bn);
          return std::make_shared <op_subx> (upstream, origin, op,
                                             t.cst ().value ().uval ());
        }

      case tree_type::CLOSE_STAR:
        {
          auto origin = std::make_shared <op_origin> ();
          auto op = build_exec (t.child (0), origin, bn);
          return std::make_shared <op_tr_closure> (upstream, origin, op,
                                                   op_tr_closure_kind::star);
        }

      case tree_type::CLOSE_PLUS:
        {
          auto origin = std::make_shared <op_origin> ();
          auto op = build_exec (t.child (0), origin, bn);
          return std::make_shared <op_tr_closure> (upstream, origin, op,
                                                   op_tr_closure_kind::plus);
        }

      case tree_type::SCOPE:
	{
	  bindings scope {bn};
	  return build_exec (t.child (0), upstream, scope);
	}

      case tree_type::BLOCK:
	{
	  // Figure out which names the block references.
	  std::vector <std::string> env_names;
	  {
	    bindings pseudo_bn;
	    for (std::string const &name: bn.names_closure ())
	      pseudo_bn.bind (name, std::make_shared <op_bind> (nullptr));
	    // OP is only necessary for keeping the references from inside the
	    // block alive.
	    auto op = build_exec (t.child (0), nullptr, pseudo_bn);
	    for (std::string const &name: pseudo_bn.names ())
	      {
		std::shared_ptr <op_bind> bnd = pseudo_bn.find (name);
		// One reference from the pseudo_bn dictionary, one reference
		// form the variable BND, anything more means the binding is
		// referenced from inside the block.
		if (bnd.use_count () > 2)
		  env_names.push_back (name);
	      }
	  }

	  // Replace the environment bindings with pseudos.
	  rendezvous rdv;
	  std::vector <std::shared_ptr <pseudo_bind>> pseudos;
	  bindings pseudo_bn;
	  {
	    unsigned id = 0;
	    for (std::string const &name: env_names)
	      {
		std::shared_ptr <op_bind> src = bn.find (name);
		auto pseudo = std::make_shared <pseudo_bind> (rdv, src, id++);
		pseudos.push_back (pseudo);
		pseudo_bn.bind (name, pseudo);
	      }
	  }

	  // Finally, op_lex_closure can be built.
	  auto origin = std::make_shared <op_origin> ();
	  auto op = build_exec (t.child (0), origin, pseudo_bn);

	  return std::make_shared <op_lex_closure> (upstream, origin, op,
						    pseudos, rdv);

	  // - When op_lex_closure is executed, it goes through the list of
	  //   pseudos and fetches values from actual source. It then builds a
	  //   closure value, which:
	  //   - holds those values in an array suitable for indexing
	  //     by pseudo id
	  //   - knows about the rendezvous
	  //   - holds origin and op with the actual compiled program
	  //
	  // - When op_apply is executed, it:
	  //   - unless it was primed
	  //     - fetches stack from its upstream and primes origin
	  //   - remembers closure value currently at rendezvous
	  //   - overwrites rendezvous with closure value on TOS
	  //   - tries to pull stack from op
	  //   - replaces previous rendezvous value
	  //   - yields if op yielded a stack
	  //   - otherwise unprimes origin and retries
	  //
	  // - op_pseudo is a type of binding (op_bind::current is virtual).
	  //   When op_pseudo is executed, it:
	  //   - reads stack from upstream
	  //   - reads closure currently at rendezvous
	  //   - reads value in that closure's array according to this
	  //     pseudo's index
	  //   - pushes that to upstream stack and yields
	  //
	  // The reason for all this complexity is that with expressions such as
	  // this:
	  //
	  //  [let A := 1, 2; {A}] (|A| A elem ?0 A elem ?1) swap? apply
	  //
	  // ... we end up with a stack of two closure values that are identical
	  // in many ways: the same piece of syntax specified the code, the same
	  // op_lex_closure built the value, and the same apply will end up
	  // calling them, yet each of them must return a different value.
	  // Previously, this was handled by storing a tree at closure value,
	  // and having apply compile in-place. I want to avoid that.
	  //
	  // Another wrinkle is that several closures of the same type
	  // (identical as per the above paragraph) might end up being in
	  // recursive call chain.
	}

      case tree_type::BIND:
        {
          auto ret = std::make_shared <op_bind> (upstream);
          bn.bind (t.str (), ret);
          return ret;
        }

      case tree_type::READ:
        {
          auto src = bn.find (t.str ());
          return std::make_shared <op_read> (upstream, *src);
        }

      case tree_type::F_DEBUG:
        return std::make_shared <op_f_debug> (upstream);

      case tree_type::IFELSE:
        {
          auto cond_origin = std::make_shared <op_origin> ();
          auto cond_op = build_exec (t.child (0), cond_origin, bn);

          auto then_origin = std::make_shared <op_origin> ();
          auto then_op = build_exec (t.child (1), then_origin, bn);

          auto else_origin = std::make_shared <op_origin> ();
          auto else_op = build_exec (t.child (2), else_origin, bn);

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
tree::build_exec (std::shared_ptr <op> upstream) const
{
  bindings root;
  return ::build_exec (*this, upstream, root);
}
