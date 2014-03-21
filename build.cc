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
      return std::make_unique <pred_eq> (slot () - 1, slot ());

    case tree_type::PRED_NE:
      return std::make_unique <pred_not>
	(std::make_unique <pred_eq> (slot () - 1, slot ()));

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
      return std::make_unique <pred_root> (slot ());

    case tree_type::PRED_SUBX_ANY:
      assert (m_children.size () == 1);
      return std::make_unique <pred_subx_any> (m_children.front ().build ());

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

std::unique_ptr <op>
tree::build () const
{
  switch (m_tt)
    {
    case tree_type::CAT:
      {
	assert (m_children.size () > 0);
	std::unique_ptr <op> ret = m_children[0].build ();
	for (auto it = m_children.begin () + 1; it != m_children.end (); ++it)
	  ret.reset (new op_cat { std::move (ret), it->build () });
	return std::move (ret);
      }

    case tree_type::SEL_UNIVERSE:
      return std::make_unique <op_sel_universe> (slot ());

    case tree_type::NOP:
      return std::make_unique <op_nop> ();

    case tree_type::ASSERT:
      return std::make_unique <op_assert> (m_children.front ().build_pred ());

    case tree_type::F_ATVAL:
      return std::make_unique <op_f_atval> (int (m_u.cval->value ()), slot ());

    case tree_type::F_CHILD:
      return std::make_unique <op_f_child> (slot ());

    case tree_type::F_OFFSET:
      return std::make_unique <op_f_offset> (slot ());

    case tree_type::FORMAT:
      if (m_children.size () != 1
	  || m_children.front ().m_tt != tree_type::STR)
	{
	  assert (! "Format strings not yet supported.");
	  abort ();
	}

      return std::make_unique <op_format> (*m_children.front ().m_u.sval, slot ());

    case tree_type::SHF_DROP:
      return std::make_unique <op_drop> (slot ());

    case tree_type::CONST:
      {
	auto val = std::make_unique <v_uint> (m_u.cval->value ());
	return std::make_unique <op_const> (std::move (val), slot ());
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
