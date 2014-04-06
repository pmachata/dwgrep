#include <iostream>
#include <algorithm>

#include "tree.hh"
#include "make_unique.hh"

std::unique_ptr <pred>
tree::build_pred (dwgrep_graph::sptr q, size_t maxsize) const
{
  switch (m_tt)
    {
    case tree_type::PRED_TAG:
      return std::make_unique <pred_tag> (m_u.cval->value (), slot ());

    case tree_type::PRED_AT:
      return std::make_unique <pred_at> (m_u.cval->value (), slot ());

    case tree_type::PRED_NOT:
      return std::make_unique <pred_not>
	(m_children.front ().build_pred (q, maxsize));

    case tree_type::PRED_EQ:
      return std::make_unique <pred_eq> (slot () - 1, slot ());

    case tree_type::PRED_NE:
      return std::make_unique <pred_not>
	(std::make_unique <pred_eq> (slot_idx (slot ().value () - 1), slot ()));

    case tree_type::PRED_LT:
      return std::make_unique <pred_lt> (slot () - 1, slot ());

    case tree_type::PRED_GT:
      return std::make_unique <pred_gt> (slot () - 1, slot ());

    case tree_type::PRED_GE:
      return std::make_unique <pred_not>
	(std::make_unique <pred_lt> (slot () - 1, slot ()));

    case tree_type::PRED_LE:
      return std::make_unique <pred_not>
	(std::make_unique <pred_gt> (slot () - 1, slot ()));

    case tree_type::PRED_ROOT:
      return std::make_unique <pred_root> (q, slot ());

    case tree_type::PRED_SUBX_ANY:
      {
	assert (m_children.size () == 1);
	auto origin = std::make_shared <op_origin> (nullptr);
	auto op = m_children.front ().build_exec (origin, q, maxsize);
	return std::make_unique <pred_subx_any> (op, origin, maxsize);
      }

    case tree_type::PRED_EMPTY:
      return std::make_unique <pred_empty> (slot ());

    case tree_type::PRED_FIND:
    case tree_type::PRED_MATCH:
    case tree_type::PRED_SUBX_ALL:
      std::cerr << "\n\nUNHANDLED:";
      dump (std::cerr);
      std::cerr << std::endl;
      abort ();

    case tree_type::CAT:
    case tree_type::SEL_UNIVERSE:
    case tree_type::NOP:
    case tree_type::ASSERT:
    case tree_type::ALT:
    case tree_type::CAPTURE:
    case tree_type::EMPTY_LIST:
    case tree_type::TRANSFORM:
    case tree_type::PROTECT:
    case tree_type::CLOSE_PLUS:
    case tree_type::CLOSE_STAR:
    case tree_type::MAYBE:
    case tree_type::CONST:
    case tree_type::STR:
    case tree_type::FORMAT:
    case tree_type::F_ATVAL:
    case tree_type::F_ADD:
    case tree_type::F_SUB:
    case tree_type::F_MUL:
    case tree_type::F_DIV:
    case tree_type::F_MOD:
    case tree_type::F_PARENT:
    case tree_type::F_CHILD:
    case tree_type::F_ATTRIBUTE:
    case tree_type::F_PREV:
    case tree_type::F_NEXT:
    case tree_type::F_TYPE:
    case tree_type::F_OFFSET:
    case tree_type::F_NAME:
    case tree_type::F_TAG:
    case tree_type::F_FORM:
    case tree_type::F_VALUE:
    case tree_type::F_POS:
    case tree_type::F_COUNT:
    case tree_type::F_EACH:
    case tree_type::SEL_SECTION:
    case tree_type::SEL_UNIT:
    case tree_type::SHF_SWAP:
    case tree_type::SHF_DUP:
    case tree_type::SHF_OVER:
    case tree_type::SHF_ROT:
    case tree_type::SHF_DROP:
      assert (! "Should never get here.");
      abort ();
    }
  abort ();
}

std::shared_ptr <op>
tree::build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
		  size_t maxsize) const
{
  if (upstream == nullptr)
    upstream = std::make_shared <op_origin> (valfile::create (maxsize));

  switch (m_tt)
    {
    case tree_type::CAT:
      for (auto const &tree: m_children)
	upstream = tree.build_exec (upstream, q, maxsize);
      return upstream;

    case tree_type::ALT:
      {
	op_merge::opvec_t ops;
	{
	  auto f = std::make_shared <std::vector <valfile::uptr> >
	    (m_children.size ());
	  for (size_t i = 0; i < m_children.size (); ++i)
	    ops.push_back (std::make_shared <op_tine> (upstream,
						       f, i, maxsize));
	}

	auto build_branch = [&q, maxsize] (tree const &ch,
					   std::shared_ptr <op> o)
	  {
	    return ch.build_exec (o, q, maxsize);
	  };

	std::transform (m_children.begin (), m_children.end (),
			ops.begin (), ops.begin (), build_branch);

	return std::make_shared <op_merge> (ops);
      }

    case tree_type::SEL_UNIVERSE:
      return std::make_unique <op_sel_universe> (upstream, q, maxsize, slot ());

    case tree_type::NOP:
      return std::make_unique <op_nop> (upstream);

    case tree_type::ASSERT:
      return std::make_unique <op_assert>
	(upstream, m_children.front ().build_pred (q, maxsize));

    case tree_type::F_ATVAL:
      return std::make_unique <op_f_atval>
	(upstream, slot (), slot (), int (m_u.cval->value ()));

    case tree_type::F_CHILD:
      return std::make_unique <op_f_child> (upstream, maxsize,
					    slot (), slot ());

    case tree_type::F_ATTRIBUTE:
      return std::make_unique <op_f_attr> (upstream, maxsize,
					   slot (), slot ());

    case tree_type::F_OFFSET:
      return std::make_unique <op_f_offset> (upstream, slot (), slot ());

    case tree_type::F_NAME:
      return std::make_unique <op_f_name> (upstream, slot (), slot ());

    case tree_type::F_TAG:
      return std::make_unique <op_f_tag> (upstream, slot (), slot ());

    case tree_type::F_FORM:
      return std::make_unique <op_f_form> (upstream, slot (), slot ());

    case tree_type::F_VALUE:
      return std::make_unique <op_f_value> (upstream, slot (), slot ());

    case tree_type::FORMAT:
      {
	if (m_children.size () == 1
	    && m_children.front ().m_tt == tree_type::STR)
	  return std::make_unique <op_strlit>
	    (upstream, *m_children.front ().m_u.sval, slot ());

	auto s_origin = std::make_shared <stringer_origin> ();
	std::shared_ptr <stringer> strgr = s_origin;
	for (auto const &tree: m_children)
	  if (tree.m_tt == tree_type::STR)
	    strgr = std::make_shared <stringer_lit> (strgr, *tree.m_u.sval);
	  else
	    {
	      auto origin2 = std::make_shared <op_origin> (nullptr);
	      auto op = tree.build_exec (origin2, q, maxsize);
	      // N.B.: tree.slot () here is supposed to be destination
	      // slot, where the stringer_op will pick the data from.
	      strgr = std::make_shared <stringer_op>
		(strgr, origin2, op, tree.slot ());
	    }

	return std::make_shared <op_format>
	  (upstream, s_origin, strgr, slot ());
      }

    case tree_type::SHF_DROP:
      return std::make_shared <op_drop> (upstream, slot ());

    case tree_type::SHF_DUP:
      return std::make_shared <op_dup> (upstream, slot () - 1, slot ());

    case tree_type::CONST:
      return std::make_unique <op_const> (upstream, *m_u.cval, slot ());

    case tree_type::CAPTURE:
      {
	auto origin = std::make_shared <op_origin> (nullptr);
	auto op = m_children.front ().build_exec (origin, q, maxsize);
	return std::make_unique <op_capture> (upstream, origin, op, maxsize,
					      m_children.front ().slot (),
					      slot ());
      }

    case tree_type::EMPTY_LIST:
      return std::make_unique <op_empty_list> (upstream, slot ());

    case tree_type::F_EACH:
      return std::make_unique <op_f_each> (upstream, maxsize, slot (), slot ());

    case tree_type::TRANSFORM:
    case tree_type::PROTECT:
    case tree_type::CLOSE_PLUS:
    case tree_type::CLOSE_STAR:
    case tree_type::MAYBE:
    case tree_type::F_ADD:
    case tree_type::F_SUB:
    case tree_type::F_MUL:
    case tree_type::F_DIV:
    case tree_type::F_MOD:
    case tree_type::F_PARENT:
    case tree_type::F_PREV:
    case tree_type::F_NEXT:
    case tree_type::F_TYPE:
    case tree_type::F_POS:
    case tree_type::F_COUNT:
    case tree_type::SEL_SECTION:
    case tree_type::SEL_UNIT:
    case tree_type::SHF_SWAP:
    case tree_type::SHF_OVER:
    case tree_type::SHF_ROT:
      std::cerr << "\n\nUNHANDLED:";
      dump (std::cerr);
      std::cerr << std::endl;
      abort ();

    case tree_type::STR:
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
    case tree_type::PRED_NOT:
    case tree_type::PRED_SUBX_ALL:
    case tree_type::PRED_SUBX_ANY:
      assert (! "Should never get here.");
      abort ();
    }

  abort ();
}
