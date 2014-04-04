#include <iostream>
#include "tree.hh"
#include "make_unique.hh"

std::unique_ptr <pred>
tree::build_pred () const
{
  switch (m_tt)
    {
    case tree_type::PRED_TAG:
      return std::make_unique <pred_tag> (m_u.cval->value (), slot ());

    case tree_type::PRED_AT:
      return std::make_unique <pred_at> (m_u.cval->value (), slot ());

    case tree_type::PRED_NOT:
      return std::make_unique <pred_not> (m_children.front ().build_pred ());

    case tree_type::PRED_EQ:
      return std::make_unique <pred_eq>
	(slot_idx (slot ().value () - 1), slot ());

    case tree_type::PRED_NE:
      //return std::make_unique <pred_not>
      //(std::make_unique <pred_eq> (slot () - 1, slot ()));
      assert (! "NIY");

    case tree_type::PRED_LT:
      //return std::make_unique <pred_lt> (slot () - 1, slot ());
      assert (! "NIY");

    case tree_type::PRED_GT:
      //return std::make_unique <pred_gt> (slot () - 1, slot ());
      assert (! "NIY");

    case tree_type::PRED_GE:
      //return std::make_unique <pred_not>
      //(std::make_unique <pred_lt> (slot () - 1, slot ()));
      assert (! "NIY");

    case tree_type::PRED_LE:
      //return std::make_unique <pred_not>
      //(std::make_unique <pred_gt> (slot () - 1, slot ()));
      assert (! "NIY");

    case tree_type::PRED_ROOT:
      //return std::make_unique <pred_root> (slot ());
      assert (! "NIY");

    case tree_type::PRED_SUBX_ANY:
      assert (m_children.size () == 1);
      //return std::make_unique <pred_subx_any> (m_children.front ().build ());
      assert (! "NIY");

    case tree_type::PRED_FIND:
    case tree_type::PRED_MATCH:
    case tree_type::PRED_EMPTY:
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
tree::build_exec (std::shared_ptr <op> upstream, dwgrep_graph::ptr q,
		  size_t maxsize) const
{
  if (upstream == nullptr)
    upstream = std::make_shared <op_origin> ();

  switch (m_tt)
    {
    case tree_type::CAT:
      for (auto const &tree: m_children)
	upstream = tree.build_exec (upstream, q, maxsize);
      return upstream;

    case tree_type::SEL_UNIVERSE:
      // XXX the 0 means that the old stack has size of 0.  That is
      // something that determine_stack_effects should determine.
      // Here we assume that ustream is op_origin.
      assert (std::dynamic_pointer_cast <op_origin> (upstream) != nullptr);
      return std::make_unique <op_sel_universe> (upstream, q,
						 0, maxsize, slot ());

    case tree_type::NOP:
      return std::make_unique <op_nop> (upstream);

    case tree_type::ASSERT:
      return std::make_unique <op_assert> (upstream,
					   m_children.front ().build_pred ());

    case tree_type::F_ATVAL:
      return std::make_unique <op_f_atval> (upstream,
					    int (m_u.cval->value ()), slot ());

    case tree_type::F_CHILD:
      return std::make_unique <op_f_child> (upstream, q, maxsize, maxsize,
					    slot (), slot ());

    case tree_type::F_OFFSET:
      return std::make_unique <op_f_offset> (upstream, slot (), slot ());

    case tree_type::FORMAT:
      if (m_children.size () != 1
	  || m_children.front ().m_tt != tree_type::STR)
	{
	  assert (! "Format strings not yet supported.");
	  abort ();
	}

      assert (! "NIY");
      //return std::make_unique <op_format> (*m_children.front ().m_u.sval, slot ());

    case tree_type::SHF_DROP:
      //return std::make_unique <op_drop> (slot ());
      assert (! "NIY");

    case tree_type::CONST:
      {
	auto val = std::make_unique <value_cst> (constant (*m_u.cval));
	return std::make_unique <op_const> (upstream, std::move (val), slot ());
      }

    case tree_type::PRED_TAG:
    case tree_type::ALT:
    case tree_type::CAPTURE:
    case tree_type::TRANSFORM:
    case tree_type::PROTECT:
    case tree_type::CLOSE_PLUS:
    case tree_type::CLOSE_STAR:
    case tree_type::MAYBE:
    case tree_type::STR:
    case tree_type::F_ADD:
    case tree_type::F_SUB:
    case tree_type::F_MUL:
    case tree_type::F_DIV:
    case tree_type::F_MOD:
    case tree_type::F_PARENT:
    case tree_type::F_ATTRIBUTE:
    case tree_type::F_PREV:
    case tree_type::F_NEXT:
    case tree_type::F_TYPE:
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
      std::cerr << "\n\nUNHANDLED:";
      dump (std::cerr);
      std::cerr << std::endl;
      abort ();

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
