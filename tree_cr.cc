#include "tree_cr.hh"

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
tree::create_protect (tree *t)
{
  return tree::create_unary <tree_type::PROTECT> (t);
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
}


tree
tree::build_block (tree t, std::shared_ptr <scope> scope)
{
  if (scope->empty ())
    return t;
  else
    {
      tree ret {tree_type::BLOCK, scope};
      ret.m_children.push_back (t);
      return ret;
    }
}

namespace
{
  tree
  promote_scopes (tree t, std::shared_ptr <scope> scp)
  {
    switch (t.tt ())
      {
      case tree_type::ALT:
      case tree_type::CAPTURE:
      case tree_type::BLOCK:
      case tree_type::PROTECT:
      case tree_type::CLOSE_STAR:
      case tree_type::ASSERT:
	for (auto &c: t.m_children)
	  c = tree::promote_scopes (c, scp);
	return t;

      case tree_type::BIND:
	if (scp->has_name (t.str ()))
	  throw std::runtime_error (std::string {"Name `"}
				    + t.str () + "' rebound.");
	scp->add_name (t.str ());
	assert (t.m_children.size () == 0);
	return t;

      case tree_type::CAT: case tree_type::READ: case tree_type::EMPTY_LIST:
      case tree_type::TRANSFORM: case tree_type::NOP: case tree_type::PRED_AT:
      case tree_type::PRED_TAG: case tree_type::PRED_EQ:
      case tree_type::PRED_NE: case tree_type::PRED_GT:
      case tree_type::PRED_GE: case tree_type::PRED_LT:
      case tree_type::PRED_LE: case tree_type::PRED_FIND:
      case tree_type::PRED_MATCH: case tree_type::PRED_EMPTY:
      case tree_type::PRED_ROOT: case tree_type::PRED_AND:
      case tree_type::PRED_OR: case tree_type::PRED_NOT:
      case tree_type::PRED_SUBX_ALL: case tree_type::PRED_SUBX_ANY:
      case tree_type::PRED_LAST: case tree_type::CONST: case tree_type::STR:
      case tree_type::FORMAT: case tree_type::F_ADD: case tree_type::F_SUB:
      case tree_type::F_MUL: case tree_type::F_DIV: case tree_type::F_MOD:
      case tree_type::F_PARENT: case tree_type::F_CHILD:
      case tree_type::F_ATTRIBUTE: case tree_type::F_ATTR_NAMED:
      case tree_type::F_PREV: case tree_type::F_NEXT: case tree_type::F_TYPE:
      case tree_type::F_OFFSET: case tree_type::F_NAME: case tree_type::F_TAG:
      case tree_type::F_FORM: case tree_type::F_VALUE: case tree_type::F_POS:
      case tree_type::F_EACH: case tree_type::F_LENGTH: case tree_type::F_CAST:
      case tree_type::SEL_UNIVERSE: case tree_type::SEL_WINFO:
      case tree_type::SEL_SECTION: case tree_type::SEL_UNIT:
      case tree_type::SHF_SWAP: case tree_type::SHF_DUP:
      case tree_type::SHF_OVER: case tree_type::SHF_ROT:
      case tree_type::SHF_DROP:
	for (auto &c: t.m_children)
	  c = ::promote_scopes (c, scp);
	return t;
      }
    return t;
  }
}

tree
tree::promote_scopes (tree t, std::shared_ptr <scope> parent)
{
  auto scp = std::make_shared <scope> (parent);
  auto t2 = ::promote_scopes (t, scp);
  return tree::build_block (t2, scp);
}
