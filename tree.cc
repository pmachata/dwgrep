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
  : tree (static_cast <tree_type> (-1))
{}

tree::tree (tree_type tt)
  : m_tt (tt)
  , m_src_a (-1)
  , m_src_b (-1)
  , m_dst (-1)
{}

tree::tree (tree const &other)
  : m_tt (other.m_tt)
  , m_children (other.m_children)
  , m_src_a (other.m_src_a)
  , m_src_b (other.m_src_b)
  , m_dst (other.m_dst)
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
  std::swap (m_src_a, other.m_src_a);
  std::swap (m_src_b, other.m_src_b);
  std::swap (m_dst, other.m_dst);
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

  if (m_src_a != -1 || m_src_b != -1 || m_dst != -1)
    {
      o << " [";
      if (m_src_a != -1)
	o << "a=" << m_src_a << ";";
      if (m_src_b != -1)
	o << "b=" << m_src_b << ";";
      if (m_dst != -1)
	o << "dst=" << m_dst << ";";
      o << "]";
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
  struct stack_refs
  {
    std::vector <ssize_t> m_freelist;

  public:
    std::vector <ssize_t> stk;

    bool
    operator== (stack_refs other) const
    {
      return m_freelist == other.m_freelist
	&& stk == other.stk;
    }

    void
    push ()
    {
      if (! m_freelist.empty ())
	{
	  stk.push_back (m_freelist.back ());
	  m_freelist.pop_back ();
	}
      else
	stk.push_back (stk.size ());
    }

    size_t
    max ()
    {
      return m_freelist.size () + stk.size ();
    }

    void
    accomodate (stack_refs other)
    {
      while (max () < other.max ())
	m_freelist.push_back (max ());
    }

    void
    drop (unsigned i = 1)
    {
      while (i-- > 0)
	{
	  if (stk.size () == 0)
	    throw std::runtime_error ("stack underrun");
	  m_freelist.push_back (stk.back ());
	  stk.pop_back ();
	}
    }

    void
    swap ()
    {
      if (stk.size () < 2)
	throw std::runtime_error ("stack underrun");

      std::swap (stk[stk.size () - 2], stk[stk.size () - 1]);
    }

    void
    rot ()
    {
      if (stk.size () < 3)
	throw std::runtime_error ("stack underrun");

      ssize_t idx = stk[stk.size () - 3];
      stk.erase (stk.begin () + stk.size () - 3);
      stk.push_back (idx);
    }

    ssize_t
    top ()
    {
      if (stk.size () < 1)
	throw std::runtime_error ("stack underrun");

      return stk.back ();
    }

    ssize_t
    below ()
    {
      if (stk.size () < 2)
	throw std::runtime_error ("stack underrun");

      return stk[stk.size () - 2];
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

  stack_refs
  resolve_operands (tree &t, stack_refs sr, bool elim_shf,
		    bool protect = false)
  {
    switch (t.m_tt)
      {
      case tree_type::CLOSE_PLUS:
	  {
	    assert (t.m_children.size () == 1);
	    tree t2 {tree_type::CAT};
	    t2.m_children.push_back (t.m_children[0]);
	    tree t3 = t;
	    t3.m_tt = tree_type::CLOSE_STAR;
	    t2.m_children.push_back (std::move (t3));
	    t = t2;
	  }
	  // Fall through.  T became a CAT node.

      case tree_type::CAT:
	for (auto &t1: t.m_children)
	  sr = resolve_operands (t1, sr, elim_shf);
	break;

      case tree_type::MAYBE:
	{
	  assert (t.m_children.size () == 1);
	  tree t2 {tree_type::ALT};
	  t2.m_children.push_back (t.m_children[0]);
	  t2.m_children.push_back (tree {tree_type::NOP});
	  t = t2;
	}
	// Fall through.  T became an ALT node.

      case tree_type::ALT:
	{
	  assert (t.m_children.size () >= 2);

	  // First try to resolve operands with the assumption that
	  // all branches have the same stack effect, i.e. try to
	  // replace shuffling of stack slots with shuffling of stack
	  // indices.  If the branches don't have identical effects,
	  // fall back to conservative approach of preserving stack
	  // shuffling.  Reject cases where stack effects are
	  // unbalanced (e.g. (,dup), (,drop) etc.).

	  auto nchildren = t.m_children;
	  stack_refs sr2 = resolve_operands (nchildren.front (), sr, true);
	  if (std::all_of (nchildren.begin () + 1, nchildren.end (),
			   [&sr2, sr] (tree &t)
			   {
			     auto sr3 = resolve_operands (t, sr, true);
			     if (sr3.stk.size () != sr2.stk.size ())
			       throw std::runtime_error
				 ("unbalanced stack effects");
			     sr2.accomodate (sr3);
			     return sr3 == sr2;
			   }))
	    {
	      t.m_children = std::move (nchildren);
	      sr = sr2;
	    }
	  else
	    {
	      sr2 = resolve_operands (t.m_children.front (), sr, false);
	      std::for_each (t.m_children.begin () + 1, t.m_children.end (),
			     [&sr2, sr] (tree &t)
			     {
			       sr2.accomodate (resolve_operands (t, sr, false));
			     });
	      sr = sr2;
	    }

	  break;
	}

      case tree_type::CAPTURE:
	{
	  assert (t.m_children.size () == 1);
	  auto sr2 = resolve_operands (t.m_children[0], sr, true);
	  t.m_src_a = sr2.top ();

	  sr.accomodate (sr2);
	  sr.push ();
	  t.m_dst = sr.top ();
	  break;
	}

      case tree_type::SEL_UNIVERSE:
      case tree_type::CONST:
      case tree_type::EMPTY_LIST:
	sr.push ();
	t.m_dst = sr.top ();
	break;

      case tree_type::FORMAT:
	for (auto &t1: t.m_children)
	  if (t1.m_tt != tree_type::STR)
	    {
	      sr = resolve_operands (t1, sr, elim_shf);

	      // Some nodes don't have a destination slot (e.g. NOP,
	      // ASSERT), but we need somewhere to look for the result
	      // value.  So if a node otherwise doesn't touch the
	      // stack, mark TOS at its m_dst.
	      if (t1.m_dst != -1)
		assert (t1.m_dst == sr.top ());
	      else
		t1.m_dst = sr.top ();

	      sr.drop ();
	    }
	sr.push ();
	t.m_dst = sr.top ();
	break;

      // STR should be handled specially in FORMAT.
      case tree_type::STR:
	assert (false);
	abort ();

      case tree_type::F_ADD:
      case tree_type::F_SUB:
      case tree_type::F_MUL:
      case tree_type::F_DIV:
      case tree_type::F_MOD:
	t.m_src_a = sr.below ();
	t.m_src_b = sr.top ();
	if (! protect)
	  sr.drop (2);
	sr.push ();
	t.m_dst = sr.top ();
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
	t.m_src_a = sr.top ();
	if (! protect)
	  sr.drop ();
	sr.push ();
	t.m_dst = sr.top ();
	break;

      case tree_type::PROTECT:
	assert (t.m_children.size () == 1);
	sr = resolve_operands (t.m_children[0], sr, elim_shf, true);
	break;

      case tree_type::NOP:
	break;

      case tree_type::ASSERT:
	assert (t.m_children.size () == 1);
	sr.accomodate (resolve_operands (t.m_children[0], sr, elim_shf));
	break;

      case tree_type::SHF_SWAP:
	if (elim_shf)
	  {
	    t.m_tt = tree_type::NOP;
	    sr.swap ();
	  }
	else
	  {
	    t.m_src_a = sr.below ();
	    t.m_dst = sr.top ();
	  }
	break;

      case tree_type::SHF_DUP:
	t.m_src_a = sr.top ();
	sr.push ();
	t.m_dst = sr.top ();
	break;

      case tree_type::SHF_OVER:
	t.m_src_a = sr.below ();
	sr.push ();
	t.m_dst = sr.top ();
	break;

      case tree_type::SHF_ROT:
	assert (! "resolve_operands: ROT unhandled");
	abort ();
	break;

      case tree_type::TRANSFORM:
	{
	  assert (t.m_children.size () == 2);
	  assert (t.m_children.front ().m_tt == tree_type::CONST);
	  assert (t.m_children.front ().m_u.cval->dom ()
		  == &unsigned_constant_dom);

	  if (t.m_children.back ().m_tt == tree_type::TRANSFORM)
	    throw std::runtime_error ("directly nested X/ disallowed");

	  // OK, now we translate N/E into N of E's, each operating in
	  // a different depth.
	  uint64_t depth = t.m_children.front ().m_u.cval->value ();
	  sr.drop (depth);

	  std::vector <tree> nchildren;
	  for (uint64_t i = 0; i < depth; ++i)
	    {
	      sr.push ();
	      nchildren.push_back (t.m_children.back ());
	      sr = resolve_operands (nchildren.back (), sr, elim_shf);
	    }

	  t.m_children = nchildren;
	  t.m_tt = tree_type::CAT;
	  break;
	}

      case tree_type::CLOSE_STAR:
	{
	  assert (t.m_children.size () == 1);
	  auto sr2 = resolve_operands (t.m_children[0], sr, elim_shf);
	  if (sr2.stk.size () != sr.stk.size ())
	    throw std::runtime_error
	      ("iteration doesn't have neutral stack effect");
	  sr = std::move (sr2);
	  break;
	}

      case tree_type::SHF_DROP:
	t.m_dst = sr.top ();
	sr.drop ();
	break;

      case tree_type::PRED_AT:
      case tree_type::PRED_TAG:
      case tree_type::PRED_EMPTY:
      case tree_type::PRED_ROOT:
	assert (t.m_children.size () == 0);
	t.m_src_a = sr.top ();
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
	t.m_src_a = sr.below ();
	t.m_src_b = sr.top ();
	break;

      case tree_type::PRED_NOT:
	assert (t.m_children.size () == 1);
	sr.accomodate (resolve_operands (t.m_children[0], sr, elim_shf));
	break;

      case tree_type::PRED_SUBX_ALL:
      case tree_type::PRED_SUBX_ANY:
	{
	  assert (t.m_children.size () == 1);
	  sr.accomodate (resolve_operands (t.m_children[0], sr, true));
	  break;
	}

      case tree_type::PRED_AND:
      case tree_type::PRED_OR:
	assert (t.m_children.size () == 2);
	sr.accomodate (resolve_operands (t.m_children[1],
					 resolve_operands (t.m_children[0], sr,
							   elim_shf),
					 elim_shf));
	break;
      }

    return sr;
  }

  ssize_t
  get_common_slot (tree const &t)
  {
    switch (t.m_tt)
      {
      case tree_type::PRED_AT:
      case tree_type::PRED_TAG:
      case tree_type::PRED_EMPTY:
      case tree_type::PRED_ROOT:
	return t.m_src_a;

      case tree_type::PRED_AND:
      case tree_type::PRED_OR:
	{
	  assert (t.m_children.size () == 2);
	  ssize_t a = get_common_slot (t.m_children[0]);
	  ssize_t b = get_common_slot (t.m_children[1]);
	  if (a == -1 || b == -1)
	    return -1;
	  return a;
	}

      case tree_type::PRED_NOT:
	assert (t.m_children.size () == 1);
	return get_common_slot (t.m_children[0]);

      case tree_type::PRED_SUBX_ALL: case tree_type::PRED_SUBX_ANY:
      case tree_type::PRED_FIND: case tree_type::PRED_MATCH:
      case tree_type::PRED_EQ: case tree_type::PRED_NE: case tree_type::PRED_GT:
      case tree_type::PRED_GE: case tree_type::PRED_LT: case tree_type::PRED_LE:
	return -1;

      case tree_type::CAT: case tree_type::ALT: case tree_type::CAPTURE:
      case tree_type::EMPTY_LIST: case tree_type::TRANSFORM:
      case tree_type::PROTECT: case tree_type::NOP:
      case tree_type::CLOSE_PLUS: case tree_type::CLOSE_STAR:
      case tree_type::MAYBE: case tree_type::ASSERT: case tree_type::CONST:
      case tree_type::STR: case tree_type::FORMAT: case tree_type::F_ATVAL:
      case tree_type::F_ADD: case tree_type::F_SUB: case tree_type::F_MUL:
      case tree_type::F_DIV: case tree_type::F_MOD: case tree_type::F_PARENT:
      case tree_type::F_CHILD: case tree_type::F_ATTRIBUTE:
      case tree_type::F_PREV: case tree_type::F_NEXT: case tree_type::F_TYPE:
      case tree_type::F_OFFSET: case tree_type::F_NAME: case tree_type::F_TAG:
      case tree_type::F_FORM: case tree_type::F_VALUE: case tree_type::F_POS:
      case tree_type::F_COUNT: case tree_type::F_EACH:
      case tree_type::SEL_UNIVERSE: case tree_type::SEL_SECTION:
      case tree_type::SEL_UNIT: case tree_type::SHF_SWAP:
      case tree_type::SHF_DUP: case tree_type::SHF_OVER:
      case tree_type::SHF_ROT: case tree_type::SHF_DROP:
	assert (! "Should never gete here.");
	abort ();
      };

    assert (t.m_tt != t.m_tt);
    abort ();
  }
}

size_t
tree::determine_stack_effects ()
{
  size_t ret = resolve_operands (*this, stack_refs {}, true).max ();
  dump (std::cerr);
  std::cerr << std::endl;
  return ret;
}

void
tree::simplify ()
{
  // Promote CAT's in CAT nodes and ALT's in ALT nodes.  Parser does
  // this already, but other transformations may lead to re-emergence
  // of this pattern.
  if (m_tt == tree_type::CAT || m_tt == tree_type::ALT)
    while (true)
      {
	bool changed = false;
	for (size_t i = 0; i < m_children.size (); )
	  if (m_children[i].m_tt == m_tt)
	    {
	      std::vector <tree> nchildren = m_children;

	      tree tmp = std::move (nchildren[i]);
	      nchildren.erase (nchildren.begin () + i);
	      nchildren.insert (nchildren.begin () + i,
				tmp.m_children.begin (),
				tmp.m_children.end ());

	      m_children = std::move (nchildren);
	      changed = true;
	    }
	  else
	    ++i;

	if (! changed)
	  break;
      }

  // Promote PROTECT's child or CAT's only child.
  if (m_tt == tree_type::PROTECT
      || (m_tt == tree_type::CAT && m_children.size () == 1))
    *this = m_children.front ();

  // Change (DUP[a=A;dst=B;] [...] X[a=B;dst=B;]) to (X[a=A;dst=B] [...]).
  for (size_t i = 0; i < m_children.size (); ++i)
    if (m_children[i].m_tt == tree_type::SHF_DUP)
      for (size_t j = i + 1; j < m_children.size (); ++j)
	if (m_children[j].m_src_b == -1
	    && m_children[j].m_src_a == m_children[i].m_dst
	    && m_children[j].m_dst == m_children[i].m_dst)
	  {
	    if (false)
	      {
		std::cerr << "dup: ";
		m_children[i].dump (std::cerr);
		std::cerr << std::endl;
		std::cerr << "  x: ";
		m_children[j].dump (std::cerr);
		std::cerr << std::endl;
	      }

	    m_children[j].m_src_a = m_children[i].m_src_a;
	    m_children[i] = std::move (m_children[j]);
	    m_children.erase (m_children.begin () + j);
	    break;
	  }

  // Recurse.
  for (auto &t: m_children)
    t.simplify ();

  // Convert ALT->(ASSERT->PRED, ASSERT->PRED) to
  // ASSERT->OR->(PRED, PRED), if all PRED's are on the same slot.
  if (m_tt == tree_type::ALT
      && std::all_of (m_children.begin (), m_children.end (),
		      [] (tree const &t) {
			return t.m_tt == tree_type::ASSERT;
		      }))
    {
      tree t = std::move (m_children[0].m_children[0]);
      ssize_t a = get_common_slot (t);
      if (a != -1
	  && std::all_of (m_children.begin () + 1, m_children.end (),
			  [a] (tree &ch) {
			    return a == get_common_slot (ch.m_children[0]);
			  }))
	{
	  std::for_each (m_children.begin () + 1, m_children.end (),
			 [&t] (tree &ch) {
			   tree t2 { tree_type::PRED_OR };
			   t2.m_children.push_back (std::move (t));
			   t2.m_children.push_back
			     (std::move (ch.m_children[0]));
			   t = std::move (t2);
			 });

	  tree u { tree_type::ASSERT };
	  u.m_children.push_back (std::move (t));

	  *this = std::move (u);
	}
    }


  // Move assertions as close to their producing node as possible.
  if (m_tt == tree_type::CAT)
    for (size_t i = 1; i < m_children.size (); ++i)
      if (m_children[i].m_tt == tree_type::ASSERT)
	{
	  ssize_t a = get_common_slot (m_children[i].m_children[0]);
	  if (a == -1)
	    continue;

	  for (ssize_t j = i - 1; j >= 0; --j)
	    if (m_children[j].m_dst == a)
	      {
		j += 1;
		assert (j >= 0);
		if ((size_t) j == i)
		  break;

		if (false)
		  {
		    std::cerr << "assert: ";
		    m_children[i].dump (std::cerr);
		    std::cerr << std::endl;
		    std::cerr << "     p: ";
		    m_children[j].dump (std::cerr);
		    std::cerr << std::endl;
		  }

		tree t = std::move (m_children[i]);
		m_children.erase (m_children.begin () + i);
		m_children.insert (m_children.begin () + j, std::move (t));
		break;
	      }
	}

  if (m_tt == tree_type::CAT)
    // Drop NOP's in CAT nodes.
    m_children.erase (std::remove_if (m_children.begin (), m_children.end (),
				      [] (tree &t) {
					return t.m_tt == tree_type::NOP;
				      }),
		      m_children.end ());
}
