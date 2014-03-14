#include <cassert>
#include <iostream>
#include <stdexcept>
#include <climits>

#include "tree.hh"

namespace
{
  const tree_arity_v argtype[] = {
#define TREE_TYPE(ENUM, ARITY) tree_arity_v::ARITY,
    TREE_TYPES
#undef TREE_TYPE
  };
}

tree::tree ()
  : m_tt (static_cast <tree_type> (-1))
{}

tree::tree (tree_type tt)
  : m_tt (tt)
{}

tree::tree (tree const &other)
  : m_tt (other.m_tt)
  , m_children (other.m_children)
{
  switch (argtype[(int) m_tt])
    {
    case tree_arity_v::CONST:
      m_u.cval = new cst {*other.m_u.cval};
      break;

    case tree_arity_v::STR:
      m_u.sval = new std::string {*other.m_u.sval};
      break;

    case tree_arity_v::NULLARY:
    case tree_arity_v::UNARY:
    case tree_arity_v::BINARY:
      break;
  }
}

tree::~tree ()
{
  switch (argtype[(int) m_tt])
    {
    case tree_arity_v::NULLARY:
    case tree_arity_v::UNARY:
    case tree_arity_v::BINARY:
      break;

    case tree_arity_v::CONST:
      delete m_u.cval;
      m_u.cval = nullptr;
      break;

    case tree_arity_v::STR:
      delete m_u.sval;
      m_u.sval = nullptr;
      break;
    }
}

tree &
tree::operator= (tree other)
{
  this->swap (other);
  return *this;
}

void
tree::swap (tree &other)
{
  std::swap (m_tt, other.m_tt);
  std::swap (m_children, other.m_children);
  std::swap (m_u, other.m_u);
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
tree::append_cat (tree *t)
{
  m_children.insert (m_children.end (),
		     t->m_children.begin (), t->m_children.end ());
}

void
tree::dump (std::ostream &o) const
{
  o << "(";

  switch (m_tt)
    {
#define TREE_TYPE(ENUM, ARITY) case tree_type::ENUM: o << #ENUM; break;
      TREE_TYPES
#undef TREE_TYPE
    }

  switch (argtype[(int) m_tt])
    {
    case tree_arity_v::CONST:
      o << "<";
      m_u.cval->dom ()->format (*m_u.cval, o);
      o << ">";
      break;

    case tree_arity_v::STR:
      o << "<" << *m_u.sval << ">";
      break;

    case tree_arity_v::NULLARY:
    case tree_arity_v::UNARY:
    case tree_arity_v::BINARY:
      break;
    }

  for (auto const &child: m_children)
    {
      o << " ";
      child.dump (o);
    }

  o << ")";
}

namespace
{
  struct stack_state
  {
    unsigned emts;

    stack_state ()
      : emts (0)
    {}

    void
    push (unsigned u)
    {
      emts += u;
    }

    void
    pop (unsigned u)
    {
      if (emts < u)
	throw std::runtime_error ("stack underrun");
      emts -= u;
    }

    void
    project (int delta)
    {
      if (delta > 0)
	push (delta);
      else
	pop (-delta);
    }

    bool
    operator!= (stack_state that) const
    {
      return emts != that.emts;
    }

    int
    operator- (stack_state that) const
    {
      return emts - that.emts;
    }
  };

  std::ostream &
  operator<< (std::ostream &o, stack_state se)
  {
    return o << '<' << se.emts << '>';
  }

  stack_state
  check_tree (tree const &t, stack_state se)
  {
    switch (t.m_tt)
      {
      case tree_type::CAT:
	for (auto const &t1: t.m_children)
	  se = check_tree (t1, se);
	break;

      case tree_type::ALT:
	{
	  int delta = INT_MAX;

	  for (auto const &t1: t.m_children)
	    if (delta == INT_MAX)
	      delta = check_tree (t1, se) - se;
	    else if (delta != check_tree (t1, se) - se)
	      throw std::runtime_error ("unbalanced stack effects");

	  assert (delta != INT_MAX);
	  se.project (delta);

	  break;
	}

      case tree_type::CAPTURE:
	{
	  assert (t.m_children.size () == 1);
	  stack_state se1 = check_tree (t.m_children[0], se);
	  // We must be able to pop an element from the resulting
	  // stack.
	  se1.pop (1);
	  // And push a sequence back to main stack.
	  se.push (1);
	  break;
	}

      case tree_type::TRANSFORM:
	{
	  assert (t.m_children.size () == 2);
	  assert (t.m_children[0].m_tt == tree_type::CONST);
	  assert (t.m_children[0].m_u.cval->dom () == &untyped_cst_dom);

	  uint64_t depth = t.m_children[0].m_u.cval->value ();
	  se.pop (depth);
	  int delta = check_tree (t.m_children[1], se) - se;
	  for (uint64_t i = 0; i < depth; ++i)
	    {
	      se.push (1);
	      se.project (delta);
	    }
	  break;
	}

      case tree_type::PROTECT:
	assert (t.m_children.size () == 1);
	check_tree (t.m_children[0], se).pop (1);
	se.push (1);
	break;

      case tree_type::NOP:
	break;

      case tree_type::CLOSE_PLUS:
      case tree_type::CLOSE_STAR:
      case tree_type::MAYBE:
	assert (t.m_children.size () == 1);
	if (check_tree (t.m_children[0], se) - se != 0)
	  throw std::runtime_error
	    ("iteration doesn't have neutral stack effect");
	break;

      case tree_type::ASSERT:
	{
	  assert (t.m_children.size () == 1);
	  int delta = check_tree (t.m_children[0], se) - se;
	  // Asserts ought to have no stack effect whatsoever.  That
	  // holds for the likes of ?{} as well, as these are
	  // evaluated is sub-expression context.
	  assert (delta == 0);
	  break;
	}

      case tree_type::SEL_UNIVERSE:
      case tree_type::CONST:
	se.push (1);
	break;

      case tree_type::FORMAT:
	for (auto const &t1: t.m_children)
	  if (t1.m_tt != tree_type::STR)
	    {
	      se = check_tree (t1, se);
	      se.pop (1);
	    }
	se.push (1);
	break;

      case tree_type::F_ADD:
      case tree_type::F_SUB:
      case tree_type::F_MUL:
      case tree_type::F_DIV:
      case tree_type::F_MOD:
	se.pop (2);
	se.push (1);
	break;

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
      case tree_type::F_ATVAL:
      case tree_type::SEL_SECTION:
      case tree_type::SEL_UNIT:
	se.pop (1);
	se.push (1);
	break;

      case tree_type::SHF_SWAP:
	se.pop (2);
	se.push (2);
	break;

      case tree_type::SHF_DUP:
	se.pop (1);
	se.push (2);
	break;

      case tree_type::SHF_OVER:
	se.pop (2);
	se.push (3);
	break;

      case tree_type::SHF_ROT:
	se.pop (3);
	se.push (3);
	break;

      case tree_type::SHF_DROP:
	se.pop (1);
	break;

      // STR should be handled specially in FORMAT.
      case tree_type::STR:
	assert (false);
	abort ();

      case tree_type::PRED_AT:
      case tree_type::PRED_TAG:
      case tree_type::PRED_EMPTY:
      case tree_type::PRED_ROOT:
	assert (t.m_children.size () == 0);
	se.pop (1);
	se.push (1);
	break;

      case tree_type::PRED_EQ:
      case tree_type::PRED_NE:
      case tree_type::PRED_GT:
      case tree_type::PRED_GE:
      case tree_type::PRED_LT:
      case tree_type::PRED_LE:
      case tree_type::PRED_FIND:
      case tree_type::PRED_MATCH:
	assert (t.m_children.size () == 0);
	se.pop (2);
	se.push (2);
	break;

      case tree_type::PRED_NOT:
	assert (t.m_children.size () == 1);
	check_tree (t.m_children[0], se);
	break;

      case tree_type::PRED_SUBX_ALL:
      case tree_type::PRED_SUBX_ANY:
	assert (t.m_children.size () == 1);
	check_tree (t.m_children[0], se);
	break;
      }

    return se;
  }
}

void
tree::check () const
{
  check_tree (*this, stack_state ());
}
