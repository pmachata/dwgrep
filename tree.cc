#include <cassert>
#include <iostream>
#include <stdexcept>
#include <climits>
#include <algorithm>

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
  , m_stkslot (-1)
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
      m_u.cval = new constant {*other.m_u.cval};
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
      o << "<" << *m_u.cval << ">";
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
  struct stack_depth
  {
    unsigned elts;
    unsigned max;

    stack_depth ()
      : elts (0)
      , max (0)
    {}

    void
    push (unsigned u)
    {
      if ((elts += u) > max)
	max = elts;
    }

    void
    pop (unsigned u)
    {
      if (elts < u)
	throw std::runtime_error ("stack underrun");
      elts -= u;
    }

    void
    project (int delta, unsigned new_max)
    {
      if (delta > 0)
	push (delta);
      else
	pop (-delta);
      if (new_max > max)
	max = new_max;
    }

    bool
    operator!= (stack_depth that) const
    {
      return elts != that.elts;
    }

    int
    operator- (stack_depth that) const
    {
      return elts - that.elts;
    }
  };

  std::ostream &
  operator<< (std::ostream &o, stack_depth se)
  {
    return o << '<' << se.elts << '>';
  }

  stack_depth
  check_tree (tree &t, stack_depth se)
  {
    switch (t.m_tt)
      {
      case tree_type::CAT:
	assert (t.m_children.size () >= 2);
	for (auto &t1: t.m_children)
	  se = check_tree (t1, se);
	break;

      case tree_type::ALT:
	{
	  assert (t.m_children.size () >= 2);
	  int delta = INT_MAX;
	  unsigned max = 0;

	  for (auto &t1: t.m_children)
	    {
	      stack_depth sd1 = check_tree (t1, se);
	      max = std::max (sd1.max, max);
	      if (delta == INT_MAX)
		delta = sd1 - se;
	      else if (delta != sd1 - se)
		throw std::runtime_error ("unbalanced stack effects");
	    }

	  assert (delta != INT_MAX);
	  se.project (delta, max);
	  break;
	}

      case tree_type::CAPTURE:
	{
	  assert (t.m_children.size () == 1);
	  stack_depth se1 = check_tree (t.m_children[0], se);
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
	  assert (t.m_children[0].m_u.cval->dom () == &unsigned_constant_dom);

	  uint64_t depth = t.m_children[0].m_u.cval->value ();
	  se.pop (depth);
	  stack_depth sd1 = check_tree (t.m_children[1], se);
	  unsigned max = sd1.max - sd1.elts;
	  int delta = sd1 - se;
	  for (uint64_t i = 0; i < depth; ++i)
	    {
	      se.push (1);
	      se.project (delta, se.elts + delta + max);
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
	  auto se2 = check_tree (t.m_children[0], se);
	  se.max = std::max (se2.max, se.max);

	  // Asserts ought to have no stack effect whatsoever.  That
	  // holds for the likes of ?{} as well, as these are
	  // evaluated is sub-expression context.
	  assert (se2 - se == 0);
	  break;
	}

      case tree_type::SEL_UNIVERSE:
      case tree_type::CONST:
	se.push (1);
	break;

      case tree_type::FORMAT:
	for (auto &t1: t.m_children)
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
	t.m_stkslot = se.elts - 1;
	se.pop (1);
	return se;

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
	{
	  assert (t.m_children.size () == 1);
	  auto se2 = check_tree (t.m_children[0], se);
	  se.max = std::max (se2.max, se.max);
	  break;
	}

      case tree_type::PRED_SUBX_ALL:
      case tree_type::PRED_SUBX_ANY:
	{
	  assert (t.m_children.size () == 1);
	  auto se2 = check_tree (t.m_children[0], se);
	  se.max = std::max (se2.max, se.max);
	  break;
	}
      }

    t.m_stkslot = se.elts - 1;
    return se;
  }
}

#if 0
namespace
{
  struct stack_refs
  {
    std::vector <varindex_t> m_freelist;

  public:
    std::vector <varindex_t> stk;

    void
    push ()
    {
      if (! m_freelist.empty ())
	{
	  stk.push_back (m_freelist.back ());
	  m_freelist.pop_back ();
	}
      else
	{
	  varindex_t nv = varindex_t (stk.size ());
	  if (size_t (nv) != stk.size ())
	    throw std::runtime_error ("stack too deep");
	  stk.push_back (nv);
	}
    }

    void
    drop ()
    {
      assert (stk.size () > 0);
      m_freelist.push_back (stk.back ());
      stk.pop_back ();
    }

    void
    swap ()
    {
      std::swap (stk[stk.size () - 2], stk[stk.size () - 1]);
    }
  };

  std::ostream &
  operator<< (std::ostream &o, stack_refs const &sr)
  {
    o << "<";
    bool seen = false;
    for (auto idx: sr.stk)
      {
	o << (seen ? ";x" : "x") << (int)idx;
	seen = true;
      }
    return o << ">";
  }

  unsigned level = 0;

  stack_refs
  resolve_operands (tree &t, stack_refs sr)
  {
    for (unsigned i = 0; i < level; ++i)
      std::cout << " ";
    ++level;
    std::cout << sr << " ";
    t.dump (std::cout);
    std::cout << std::endl;

    switch (t.m_tt)
      {
      case tree_type::CAT:
	for (auto &t1: t.m_children)
	  sr = resolve_operands (t1, sr);
	break;

      case tree_type::SEL_UNIVERSE:
	sr.push ();
	break;
      }

    --level;
    for (unsigned i = 0; i < level; ++i)
      std::cout << " ";
    std::cout << sr;
    std::cout << std::endl;

    return sr;
  }
}
#endif

size_t
tree::determine_stack_effects ()
{
  return check_tree (*this, stack_depth ()).max;
}
