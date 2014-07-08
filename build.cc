#include <algorithm>
#include <iostream>
#include <memory>

#include "make_unique.hh"
#include "op.hh"
#include "tree.hh"
#include "scope.hh"

std::unique_ptr <pred>
tree::build_pred (dwgrep_graph::sptr q, std::shared_ptr <scope> scope) const
{
  switch (m_tt)
    {
    case tree_type::PRED_TAG:
      return std::make_unique <pred_tag> (cst ().value ().get_ui ());

    case tree_type::PRED_AT:
      return std::make_unique <pred_at> (cst ().value ().get_ui ());

    case tree_type::PRED_NOT:
      return std::make_unique <pred_not>
	(m_children.front ().build_pred (q, scope));

    case tree_type::PRED_OR:
      return std::make_unique <pred_or>
	(m_children[0].build_pred (q, scope),
	 m_children[1].build_pred (q, scope));

    case tree_type::PRED_AND:
      return std::make_unique <pred_and>
	(m_children[0].build_pred (q, scope),
	 m_children[1].build_pred (q, scope));

    case tree_type::PRED_EQ:
      return std::make_unique <pred_eq> ();

    case tree_type::PRED_NE:
      return std::make_unique <pred_not> (std::make_unique <pred_eq> ());

    case tree_type::PRED_LT:
      return std::make_unique <pred_lt> ();

    case tree_type::PRED_GT:
      return std::make_unique <pred_gt> ();

    case tree_type::PRED_GE:
      return std::make_unique <pred_not> (std::make_unique <pred_lt> ());

    case tree_type::PRED_LE:
      return std::make_unique <pred_not> (std::make_unique <pred_gt> ());

    case tree_type::PRED_ROOT:
      return std::make_unique <pred_root> (q);

    case tree_type::PRED_SUBX_ANY:
      {
	assert (m_children.size () == 1);
	auto origin = std::make_shared <op_origin> (nullptr);
	auto op = m_children.front ().build_exec (origin, q, scope);
	return std::make_unique <pred_subx_any> (op, origin);
      }

    case tree_type::PRED_EMPTY:
      return std::make_unique <pred_empty> ();

    case tree_type::PRED_MATCH:
      return std::make_unique <pred_match> ();

    case tree_type::F_BUILTIN:
      return m_builtin->build_pred (q, scope);

    case tree_type::PRED_FIND:
    case tree_type::PRED_LAST:
    case tree_type::PRED_SUBX_ALL:
      std::cerr << "\n\nUNHANDLED:" << *this << std::endl;
      abort ();

    case tree_type::CAT:
    case tree_type::SEL_UNIVERSE:
    case tree_type::SEL_WINFO:
    case tree_type::NOP:
    case tree_type::ASSERT:
    case tree_type::ALT:
    case tree_type::OR:
    case tree_type::CAPTURE:
    case tree_type::EMPTY_LIST:
    case tree_type::TRANSFORM:
    case tree_type::CLOSE_STAR:
    case tree_type::CONST:
    case tree_type::STR:
    case tree_type::FORMAT:
    case tree_type::F_ADD:
    case tree_type::F_SUB:
    case tree_type::F_MUL:
    case tree_type::F_DIV:
    case tree_type::F_MOD:
    case tree_type::F_PARENT:
    case tree_type::F_CHILD:
    case tree_type::F_ATTRIBUTE:
    case tree_type::F_ATTR_NAMED:
    case tree_type::F_PREV:
    case tree_type::F_NEXT:
    case tree_type::F_TYPE:
    case tree_type::F_OFFSET:
    case tree_type::F_NAME:
    case tree_type::F_TAG:
    case tree_type::F_FORM:
    case tree_type::F_VALUE:
    case tree_type::F_CAST:
    case tree_type::F_POS:
    case tree_type::F_ELEM:
    case tree_type::F_LENGTH:
    case tree_type::F_APPLY:
    case tree_type::F_DEBUG:
    case tree_type::SEL_SECTION:
    case tree_type::SEL_UNIT:
    case tree_type::SHF_SWAP:
    case tree_type::SHF_DUP:
    case tree_type::SHF_OVER:
    case tree_type::SHF_ROT:
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
tree::build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
		  std::shared_ptr <scope> scope) const
{
  if (upstream == nullptr)
    upstream = std::make_shared <op_origin> (std::make_unique <valfile> ());

  switch (m_tt)
    {
    case tree_type::CAT:
      for (auto const &tree: m_children)
	upstream = tree.build_exec (upstream, q, scope);
      return upstream;

    case tree_type::ALT:
      {
	auto done = std::make_shared <bool> (false);

	op_merge::opvec_t ops;
	{
	  auto f = std::make_shared <std::vector <valfile::uptr> >
	    (m_children.size ());
	  for (size_t i = 0; i < m_children.size (); ++i)
	    ops.push_back (std::make_shared <op_tine> (upstream, f, done, i));
	}

	auto build_branch = [&q, &scope] (tree const &ch,
					  std::shared_ptr <op> o)
	  {
	    return ch.build_exec (o, q, scope);
	  };

	std::transform (m_children.begin (), m_children.end (),
			ops.begin (), ops.begin (), build_branch);

	return std::make_shared <op_merge> (ops, done);
      }

    case tree_type::OR:
      {
	auto o = std::make_shared <op_or> (upstream);
	for (auto const &tree: m_children)
	  {
	    auto origin2 = std::make_shared <op_origin> (nullptr);
	    auto op = tree.build_exec (origin2, q, scope);
	    o->add_branch (origin2, op);
	  }
	return o;
      }

    case tree_type::SEL_WINFO:
      return std::make_shared <op_sel_winfo> (upstream, q);

    case tree_type::SEL_UNIT:
      return std::make_shared <op_sel_unit> (upstream, q);

    case tree_type::NOP:
      return std::make_shared <op_nop> (upstream);

    case tree_type::F_BUILTIN:
      {
	if (auto pred = build_pred (q, scope))
	  return std::make_shared <op_assert> (upstream, std::move (pred));
	auto op = m_builtin->build_exec (upstream, q, scope);
	assert (op != nullptr);
	return op;
      }

    case tree_type::ASSERT:
      return std::make_shared <op_assert>
	(upstream, m_children.front ().build_pred (q, scope));

    case tree_type::F_CHILD:
      return std::make_shared <op_f_child> (upstream, q);

    case tree_type::F_ATTRIBUTE:
      return std::make_shared <op_f_attr> (upstream, q);

    case tree_type::F_ATTR_NAMED:
      return std::make_shared <op_f_attr_named>
	(upstream, q, int (cst ().value ().get_ui ()));

    case tree_type::F_OFFSET:
      return std::make_shared <op_f_offset> (upstream, q);

    case tree_type::F_NAME:
      return std::make_shared <op_f_name> (upstream, q);

    case tree_type::F_TAG:
      return std::make_shared <op_f_tag> (upstream, q);

    case tree_type::F_FORM:
      return std::make_shared <op_f_form> (upstream, q);

    case tree_type::F_VALUE:
      return std::make_shared <op_f_value> (upstream, q);

    case tree_type::F_PARENT:
      return std::make_shared <op_f_parent> (upstream, q);

    case tree_type::FORMAT:
      {
	auto s_origin = std::make_shared <stringer_origin> ();
	std::shared_ptr <stringer> strgr = s_origin;
	for (auto const &tree: m_children)
	  if (tree.m_tt == tree_type::STR)
	    strgr = std::make_shared <stringer_lit> (strgr, tree.str ());
	  else
	    {
	      auto origin2 = std::make_shared <op_origin> (nullptr);
	      auto op = tree.build_exec (origin2, q, scope);
	      strgr = std::make_shared <stringer_op> (strgr, origin2, op);
	    }

	return std::make_shared <op_format> (upstream, s_origin, strgr);
      }

    case tree_type::SHF_DUP:
    case tree_type::SHF_OVER:
      return std::make_shared <op_dup> (upstream);

    case tree_type::SHF_SWAP:
      return std::make_shared <op_swap> (upstream);

    case tree_type::CONST:
      {
	auto val = std::make_unique <value_cst> (cst (), 0);
	return std::make_shared <op_const> (upstream, std::move (val));
      }

    case tree_type::STR:
      {
	auto val = std::make_unique <value_str> (std::string (str ()), 0);
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
	auto op = m_children.front ().build_exec (origin, q, scope);
	return std::make_shared <op_capture> (upstream, origin, op);
      }

    case tree_type::F_ELEM:
      return std::make_shared <op_f_elem> (upstream);

    case tree_type::F_LENGTH:
      return std::make_shared <op_f_length> (upstream);

    case tree_type::F_TYPE:
      return std::make_shared <op_f_type> (upstream);

    case tree_type::CLOSE_STAR:
      {
	auto origin = std::make_shared <op_origin> (nullptr);
	auto op = m_children.front ().build_exec (origin, q, scope);
	return std::make_shared <op_tr_closure> (upstream, origin, op);
      }

    case tree_type::F_ADD:
      return std::make_shared <op_f_add> (upstream);
    case tree_type::F_SUB:
      return std::make_shared <op_f_sub> (upstream);
    case tree_type::F_MUL:
      return std::make_shared <op_f_mul> (upstream);
    case tree_type::F_DIV:
      return std::make_shared <op_f_div> (upstream);
    case tree_type::F_MOD:
      return std::make_shared <op_f_mod> (upstream);

    case tree_type::F_POS:
      return std::make_shared <op_f_pos> (upstream);

    case tree_type::F_CAST:
      return std::make_shared <op_f_cast> (upstream, cst ().dom ());

    case tree_type::TRANSFORM:
      {
	// OK, now we translate N/E into N of E's, each operating in
	// a different depth.
	uint64_t depth = child (0).cst ().value ().get_ui ();
	assert (static_cast <unsigned> (depth) == depth);

	for (unsigned u = depth; u > 1; --u)
	  {
	    auto origin = std::make_shared <op_origin> (nullptr);
	    auto op = child (1).build_exec (origin, q, scope);
	    upstream = std::make_shared <op_transform> (upstream,
							origin, op, u);
	  }

	// Now we attach the operation itself for the case of DEPTH==1.
	return child (1).build_exec (upstream, q, scope);
      }

    case tree_type::SCOPE:
      {
	auto origin = std::make_shared <op_origin> (nullptr);
	auto op = child (0).build_exec (origin, q, scp ());
	return std::make_shared <op_scope> (upstream, origin, op,
					    scp ()->num_names ());
      }

    case tree_type::BLOCK:
      assert (scp () == nullptr);
      return std::make_shared <op_lex_closure> (upstream, child (0), q, scope);

    case tree_type::BIND:
    case tree_type::READ:
      {
	// Find access coordinates.  Stack frames form a chain.  Here
	// we find what stack frame will be accessed (i.e. how deeply
	// nested is this OP inside SCOPE's) and at which position.

	std::string const &name = str ();
	size_t depth = 0;
	for (auto scp = scope; scp != nullptr; scp = scp->parent, ++depth)
	  if (scp->has_name (name))
	    {
	      auto id = scp->index (name);
	      /*
	      std::cout << "Found `" << name << "' at depth "
			<< depth << ", index " << (size_t) id
			<< std::endl;
	      */
	      if (m_tt == tree_type::BIND)
		return std::make_shared <op_bind> (upstream, depth, id);
	      else
		return std::make_shared <op_read> (upstream, depth, id);
	    }

	throw std::runtime_error (std::string {"Unknown identifier `"}
				  + name + "'.");
      }

    case tree_type::F_APPLY:
      return std::make_shared <op_apply> (upstream);

    case tree_type::F_DEBUG:
      return std::make_shared <op_f_debug> (upstream);

    case tree_type::IFELSE:
      {
	auto cond_origin = std::make_shared <op_origin> (nullptr);
	auto cond_op = child (0).build_exec (cond_origin, q, scope);

	auto then_origin = std::make_shared <op_origin> (nullptr);
	auto then_op = child (1).build_exec (then_origin, q, scope);

	auto else_origin = std::make_shared <op_origin> (nullptr);
	auto else_op = child (2).build_exec (else_origin, q, scope);

	return std::make_shared <op_ifelse> (upstream, cond_origin, cond_op,
					     then_origin, then_op,
					     else_origin, else_op);
      }

    case tree_type::F_PREV:
    case tree_type::F_NEXT:
    case tree_type::SEL_UNIVERSE:
    case tree_type::SEL_SECTION:
    case tree_type::SHF_ROT:
      std::cerr << "\n\nUNHANDLED:" << *this << std::endl;
      abort ();

    case tree_type::PRED_TAG:
    case tree_type::PRED_AT:
    case tree_type::PRED_EQ:
    case tree_type::PRED_NE:
    case tree_type::PRED_GT:
    case tree_type::PRED_GE:
    case tree_type::PRED_LT:
    case tree_type::PRED_LE:
    case tree_type::PRED_FIND:
    case tree_type::PRED_MATCH:
    case tree_type::PRED_EMPTY:
    case tree_type::PRED_ROOT:
    case tree_type::PRED_AND:
    case tree_type::PRED_OR:
    case tree_type::PRED_NOT:
    case tree_type::PRED_SUBX_ALL:
    case tree_type::PRED_SUBX_ANY:
    case tree_type::PRED_LAST:
      assert (! "Should never get here.");
      abort ();
    }

  abort ();
}
