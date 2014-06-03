#include <cassert>
#include <iostream>
#include <stdexcept>
#include <climits>
#include <algorithm>
#include <set>
#include <boost/optional.hpp>

#include "tree.hh"

std::ostream &
operator<< (std::ostream &o, slot_coords const &c)
{
  if (c.has_src_a () || c.has_src_b () || c.has_dst ())
    {
      o << " [";
      if (c.has_src_a ())
	o << "a=" << c.src_a () << ";";
      if (c.has_src_b ())
	o << "b=" << c.src_b () << ";";
      if (c.has_dst ())
	o << "dst=" << c.dst () << ";";
      o << "]";
    }
  return o;
}

slot_idx
slot_coords::src_a () const
{
  assert (has_src_a ());
  return *m_src_a;
}

void
slot_coords::set_src_a (slot_idx idx)
{
  m_src_a = idx;
}

slot_idx
slot_coords::src_b () const
{
  assert (has_src_b ());
  return *m_src_b;
}

void
slot_coords::set_src_b (slot_idx idx)
{
  m_src_b = idx;
}

slot_idx
slot_coords::dst () const
{
  assert (has_dst ());
  return *m_dst;
}

void
slot_coords::set_dst (slot_idx idx)
{
  m_dst = idx;
}

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
  : m_tt {tt}
{}

namespace
{
  template <class T>
  std::unique_ptr <T>
  copy_unique (std::unique_ptr <T> const &ptr)
  {
    if (ptr == nullptr)
      return nullptr;
    else
      return std::make_unique <T> (*ptr);
  }
}

tree::tree (tree const &other)
  : m_str {copy_unique (other.m_str)}
  , m_cst {copy_unique (other.m_cst)}
  , m_coords {other.m_coords}
  , m_tt {other.m_tt}
  , m_children {other.m_children}
{}

tree::tree (tree_type tt, std::string const &str)
  : m_str {std::make_unique <std::string> (str)}
  , m_tt {tt}
{}

tree::tree (tree_type tt, constant const &cst)
  : m_cst {std::make_unique <constant> (cst)}
  , m_tt {tt}
{}

tree &
tree::operator= (tree other)
{
  this->swap (other);
  return *this;
}

void
tree::swap (tree &other)
{
  std::swap (m_str, other.m_str);
  std::swap (m_cst, other.m_cst);
  std::swap (m_tt, other.m_tt);
  std::swap (m_children, other.m_children);
  std::swap (m_coords, other.m_coords);
}

slot_idx
tree::src_a () const
{
  return m_coords.src_a ();
}

slot_idx
tree::src_b () const
{
  return m_coords.src_b ();
}

slot_idx
tree::dst () const
{
  return m_coords.dst ();
}

tree &
tree::child (size_t idx)
{
  assert (idx < m_children.size ());
  return m_children[idx];
}

tree const &
tree::child (size_t idx) const
{
  assert (idx < m_children.size ());
  return m_children[idx];
}

std::string &
tree::str () const
{
  assert (m_str != nullptr);
  return *m_str;
}

constant &
tree::cst () const
{
  assert (m_cst != nullptr);
  return *m_cst;
}

void
tree::push_child (tree const &t)
{
  m_children.push_back (t);
}

std::ostream &
operator<< (std::ostream &o, tree const &t)
{
  o << "(";

  switch (t.m_tt)
    {
#define TREE_TYPE(ENUM, ARITY) case tree_type::ENUM: o << #ENUM; break;
      TREE_TYPES
#undef TREE_TYPE
    }

  switch (argtype[(int) t.m_tt])
    {
    case tree_arity_v::CST:
      o << "<" << t.cst () << ">";
      break;

    case tree_arity_v::STR:
      o << "<" << t.str () << ">";
      break;

    case tree_arity_v::NULLARY:
    case tree_arity_v::UNARY:
    case tree_arity_v::BINARY:
      break;
    }

  o << t.m_coords;

  for (auto const &child: t.m_children)
    o << " " << child;

  return o << ")";
}

namespace
{
  class slot_buf
  {
    friend class stack_refs;
    std::vector <slot_idx> m_freelist;

    slot_idx
    release ()
    {
      assert (! m_freelist.empty ());
      slot_idx ret = m_freelist.front ();
      m_freelist.erase (m_freelist.begin ());
      return ret;
    }

    explicit slot_buf (std::vector <slot_idx> &&list)
      : m_freelist {std::move (list)}
    {}

    bool
    empty () const
    {
      return m_freelist.empty ();
    }

  public:
    slot_buf (slot_buf const &that) = default;
    ~slot_buf () = default;

    slot_buf
    reverse () const
    {
      std::vector <slot_idx> ret = m_freelist;
      std::reverse (ret.begin (), ret.end ());
      return slot_buf {std::move (ret)};
    }
  };

  struct stack_refs
  {
    std::vector <slot_idx> m_freelist;
    size_t m_age;
    unsigned m_max;

  public:
    stack_refs ()
      : m_age {0}
      , m_max {0}
    {}

    std::vector <slot_idx> stk;
    std::vector <size_t> age;

    bool
    operator== (stack_refs other) const
    {
      return m_freelist == other.m_freelist
	&& stk == other.stk
	&& m_max == other.m_max;
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
	stk.push_back (slot_idx (m_max++));
      age.push_back (m_age++);
    }

    void
    take_one (slot_buf &buf)
    {
      m_freelist.push_back (buf.release ());
    }

    void
    take (slot_buf &&buf)
    {
      while (! buf.empty ())
	take_one (buf);
    }

    void
    push_one (slot_buf &buf)
    {
      take_one (buf);
      push ();
    }

    size_t
    max ()
    {
      return m_max;
    }

    void
    accomodate (stack_refs other)
    {
      m_max = std::max (other.m_max, m_max);
    }

    void
    drop (unsigned i = 1)
    {
      take (drop_release (i));
    }

    slot_buf
    drop_release (unsigned i = 1)
    {
      std::vector <slot_idx> ret;
      while (i-- > 0)
	{
	  if (stk.size () == 0)
	    throw std::runtime_error ("stack underrun");
	  ret.push_back (stk.back ());
	  stk.pop_back ();
	  age.pop_back ();
	}
      return slot_buf {std::move (ret)};
    }

    void
    swap ()
    {
      if (stk.size () < 2)
	throw std::runtime_error ("stack underrun");

      std::swap (stk[stk.size () - 2], stk[stk.size () - 1]);
      std::swap (age[age.size () - 2], age[age.size () - 1]);
    }

    void
    rot ()
    {
      if (stk.size () < 3)
	throw std::runtime_error ("stack underrun");

      slot_idx e = stk[stk.size () - 3];
      stk.erase (stk.begin () + stk.size () - 3);
      stk.push_back (e);

      size_t a = age[age.size () - 3];
      age.erase (age.begin () + age.size () - 3);
      age.push_back (a);
    }

    slot_idx
    top ()
    {
      if (stk.size () < 1)
	throw std::runtime_error ("stack underrun");

      return stk.back ();
    }

    std::pair <slot_idx, size_t>
    top_w_age ()
    {
      return std::make_pair (top (), age.back ());
    }

    slot_idx
    below ()
    {
      if (stk.size () < 2)
	throw std::runtime_error ("stack underrun");

      return stk[stk.size () - 2];
    }

    std::pair <slot_idx, size_t>
    below_w_age ()
    {
      return std::make_pair (below (), age[age.size () - 2]);
    }
  };

  __attribute__ ((used)) std::ostream &
  operator<< (std::ostream &o, stack_refs const &sr)
  {
    o << "<";
    bool seen = false;
    for (auto idx: sr.stk)
      {
	o << (seen ? ";x" : "x") << idx;
	seen = true;
      }
    return o << ">";
  }

  stack_refs
  resolve_operands (tree &t, stack_refs sr, bool elim_shf)
  {
    elim_shf = false;
    switch (t.m_tt)
      {
      case tree_type::CLOSE_PLUS:
	  {
	    assert (t.m_children.size () == 1);
	    tree t2 {tree_type::CAT};
	    t2.push_child (t.child (0));
	    tree t3 = t;
	    t3.m_tt = tree_type::CLOSE_STAR;
	    t2.push_child (std::move (t3));
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
	  t2.push_child (t.child (0));
	  t2.push_child (tree {tree_type::NOP});
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
	      sr2 = resolve_operands (t.child (0), sr, false);
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

	  // Capture allows only either no change in stack effects, or
	  // one additional push.  The reason is that we need to be
	  // able to figure out where to place the empty list in case
	  // no computations are produced at all.
	  auto resolve_capture = [&t, &sr] (bool elim) -> bool
	    {
	      auto popped = [] (stack_refs sr3)
	        {
		  sr3.drop ();
		  return sr3;
		};

	      auto nchildren = t.m_children;
	      auto sr2 = resolve_operands (nchildren.front (), sr, elim);
	      if (sr2.stk == sr.stk || popped (sr2).stk == sr.stk)
		{
		  sr = sr2;
		  t.m_children = nchildren;
		  t.set_src_a (sr2.top ());
		  t.set_dst (sr2.top ());
		  return true;
		}

	      return false;
	    };

	  // Try to eliminate stack shuffling.  Retry without if it fails.
	  if (! resolve_capture (true) && ! resolve_capture (false))
	    throw std::runtime_error ("capture with too complex stack effects");

	  break;
	}

      case tree_type::SEL_WINFO:
      case tree_type::SEL_UNIVERSE:
      case tree_type::CONST:
      case tree_type::EMPTY_LIST:
      case tree_type::STR:
	sr.push ();
	t.set_dst (sr.top ());
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
	      if (t1.has_dst ())
		assert (t1.dst () == sr.top ());
	      else
		t1.set_dst (sr.top ());

	      sr.drop ();
	    }
	sr.push ();
	t.set_dst (sr.top ());
	break;

      case tree_type::F_ADD:
      case tree_type::F_SUB:
      case tree_type::F_MUL:
      case tree_type::F_DIV:
      case tree_type::F_MOD:
	{
	  auto src_a = sr.below_w_age ();
	  auto src_b = sr.top_w_age ();
	  t.set_src_a (src_a.first);
	  t.set_src_b (src_b.first);

	  // We prefer the older slot for destination.  This is a
	  // heuristic to attempt to keep stack at the exit from
	  // closure in the same state as it was on entry.  See a
	  // comment at CLOSE_STAR for more details.
	  //
	  // To achieve that, we possibly swap the two slots
	  // before dropping them, so that the following push
	  // picks up the one that we want.  Note that we
	  // extracted src_a and src_b up there, so this swapping
	  // won't influence the actual order of operands.

	  if (src_b.second < src_a.second)
	    sr.swap ();

	  sr.drop (2);
	  sr.push ();
	  t.set_dst (sr.top ());
	  break;
	}

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
      case tree_type::F_COUNT:
      case tree_type::F_EACH:
      case tree_type::F_LENGTH:
      case tree_type::SEL_SECTION:
      case tree_type::SEL_UNIT:
	t.set_src_a (sr.top ());
	sr.drop ();
	sr.push ();
	t.set_dst (sr.top ());
	break;

      case tree_type::PROTECT:
	{
	  assert (t.m_children.size () == 1);

	  auto sr2 = resolve_operands (t.child (0), sr, elim_shf);
	  t.set_src_a (sr2.top ());
	  sr2.drop ();

	  sr.accomodate (sr2);
	  sr.push ();
	  t.set_dst (sr.top ());
	  break;
	}

      case tree_type::NOP:
	break;

      case tree_type::ASSERT:
	assert (t.m_children.size () == 1);
	sr.accomodate (resolve_operands (t.child (0), sr, elim_shf));
	break;

      case tree_type::SHF_SWAP:
	if (elim_shf)
	  {
	    t.m_tt = tree_type::NOP;
	    sr.swap ();
	  }
	else
	  {
	    t.set_src_a (sr.below ());
	    t.set_dst (sr.top ());
	  }
	break;

      case tree_type::SHF_DUP:
	t.set_src_a (sr.top ());
	sr.push ();
	t.set_dst (sr.top ());
	break;

      case tree_type::SHF_OVER:
	t.set_src_a (sr.below ());
	sr.push ();
	t.set_dst (sr.top ());
	break;

      case tree_type::SHF_ROT:
	assert (! "resolve_operands: ROT unhandled");
	abort ();
	break;

      case tree_type::TRANSFORM:
	{
	  assert (t.m_children.size () == 2);
	  assert (t.child (0).m_tt == tree_type::CONST);
	  assert (t.child (0).cst ().dom () == &unsigned_constant_dom);

	  if (t.m_children.back ().m_tt == tree_type::TRANSFORM)
	    throw std::runtime_error ("directly nested X/ disallowed");

	  break;
	}

      case tree_type::CLOSE_STAR:
	{
	  // Even though the stack effects of closures have to be
	  // balanced (they are not allowed to push more than they
	  // pop, or vice versa), that doesn't mean that the stack
	  // will be in the same state after one closure iteration.
	  // For example, consider the following expression and its
	  // stack effects (the number represents the assigned stack
	  // slot):
	  //
	  //    -universe []   swap (@type [()] rot  add  swap)*
	  //    0Die      0Die 1Seq  1Seq  1Seq 0Die 0Die 2Seq
	  //              1Seq 0Die  0Die  0Die 2Seq 2Seq 0Die
	  //                               2Seq 1Seq
	  //
	  // Note how what enters the closure (1Seq/0Die) differs from
	  // what exits the closure (2Seq/0Die), even though the slot
	  // types and stack depth are the same.  The problem of
	  // course is that add doesn't know which slot to choose to
	  // make the overall closure happy.  Currently it chooses the
	  // bottom one for destination, but it would break similarly
	  // for other expressions if it chose the top one.
	  //
	  // We try to mitigate this in arithmetic operations by
	  // choosing the older stack slot for destination, but have
	  // to be conservative here as well, and possibly give up
	  // eliminating stack shuffling.

	  assert (t.m_children.size () == 1);

	  // First, optimistically try to eliminate stack shuffling
	  // and see if it happens to work out.
	  auto t2 = t.child (0);
	  auto sr2 = resolve_operands (t2, sr, elim_shf);

	  if (sr2.stk.size () != sr.stk.size ())
	    throw std::runtime_error
	      ("iteration doesn't have neutral stack effect");

	  if (sr2.stk == sr.stk)
	    {
	      // Cool, commit the changes.
	      t.child (0) = std::move (t2);
	      sr = std::move (sr2);
	    }
	  else
	    {
	      // Not cool, redo with stack shuffling.
	      sr2 = resolve_operands (t.child (0), sr, false);
	      assert (sr2.stk == sr.stk);
	      sr = std::move (sr2);
	    }

	  break;
	}

      case tree_type::SHF_DROP:
	t.set_dst (sr.top ());
	sr.drop ();
	break;

      case tree_type::PRED_AT:
      case tree_type::PRED_TAG:
      case tree_type::PRED_EMPTY:
      case tree_type::PRED_ROOT:
      case tree_type::PRED_LAST:
	assert (t.m_children.size () == 0);
	t.set_src_a (sr.top ());
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
	t.set_src_a (sr.below ());
	t.set_src_b (sr.top ());
	break;

      case tree_type::PRED_NOT:
	assert (t.m_children.size () == 1);
	sr.accomodate (resolve_operands (t.child (0), sr, elim_shf));
	break;

      case tree_type::PRED_SUBX_ALL:
      case tree_type::PRED_SUBX_ANY:
	{
	  assert (t.m_children.size () == 1);
	  sr.accomodate (resolve_operands (t.child (0), sr, true));
	  break;
	}

      case tree_type::PRED_AND:
      case tree_type::PRED_OR:
	assert (t.m_children.size () == 2);
	sr.accomodate (resolve_operands (t.child (1),
					 resolve_operands (t.child (0), sr,
							   elim_shf),
					 elim_shf));
	break;
      }

    return sr;
  }

  boost::optional <slot_idx>
  get_common_slot (tree const &t)
  {
    switch (t.m_tt)
      {
      case tree_type::PRED_AT:
      case tree_type::PRED_TAG:
      case tree_type::PRED_EMPTY:
      case tree_type::PRED_ROOT:
	return boost::make_optional (t.src_a ());

      case tree_type::PRED_AND:
      case tree_type::PRED_OR:
	{
	  assert (t.m_children.size () == 2);
	  auto a = get_common_slot (t.child (0));
	  auto b = get_common_slot (t.child (1));
	  if (! a || ! b)
	    return {};
	  return a;
	}

      case tree_type::PRED_NOT:
	assert (t.m_children.size () == 1);
	return get_common_slot (t.child (0));

      case tree_type::PRED_SUBX_ALL: case tree_type::PRED_SUBX_ANY:
      case tree_type::PRED_FIND: case tree_type::PRED_MATCH:
      case tree_type::PRED_EQ: case tree_type::PRED_NE: case tree_type::PRED_GT:
      case tree_type::PRED_GE: case tree_type::PRED_LT: case tree_type::PRED_LE:
      case tree_type::PRED_LAST:
	return {};

      case tree_type::CAT: case tree_type::ALT: case tree_type::CAPTURE:
      case tree_type::EMPTY_LIST: case tree_type::TRANSFORM:
      case tree_type::PROTECT: case tree_type::NOP:
      case tree_type::CLOSE_PLUS: case tree_type::CLOSE_STAR:
      case tree_type::MAYBE: case tree_type::ASSERT: case tree_type::CONST:
      case tree_type::STR: case tree_type::FORMAT:
      case tree_type::F_ADD: case tree_type::F_SUB: case tree_type::F_MUL:
      case tree_type::F_DIV: case tree_type::F_MOD: case tree_type::F_PARENT:
      case tree_type::F_CHILD:
      case tree_type::F_ATTRIBUTE: case tree_type::F_ATTR_NAMED:
      case tree_type::F_PREV: case tree_type::F_NEXT: case tree_type::F_TYPE:
      case tree_type::F_OFFSET: case tree_type::F_NAME: case tree_type::F_TAG:
      case tree_type::F_FORM: case tree_type::F_VALUE: case tree_type::F_POS:
      case tree_type::F_CAST:
      case tree_type::F_COUNT: case tree_type::F_EACH: case tree_type::F_LENGTH:
      case tree_type::SEL_UNIVERSE: case tree_type::SEL_SECTION:
      case tree_type::SEL_WINFO:
      case tree_type::SEL_UNIT: case tree_type::SHF_SWAP:
      case tree_type::SHF_DUP: case tree_type::SHF_OVER:
      case tree_type::SHF_ROT: case tree_type::SHF_DROP:
	assert (! "Should never get here.");
	abort ();
      };

    assert (t.m_tt != t.m_tt);
    abort ();
  }

  std::set <slot_idx>
  resolve_count (tree &t, std::set <slot_idx> unresolved)
  {
    // Called for nodes that can resolve one of the unresolved slots.
    auto can_resolve = [&unresolved, &t] ()
      {
	slot_idx dst = t.dst ();
	if (unresolved.find (dst) != unresolved.end ())
	  {
	    if (true)
	      std::cerr << "found producer: " << t << std::endl;

	    // Convert the producer X to (CAT (CAPTURE X) (EACH)).
	    tree t2 {tree_type::CAT};

	    tree t3 {tree_type::CAPTURE};
	    t3.push_child (t);
	    t3.set_src_a (dst);
	    t3.set_dst (dst);
	    t2.push_child (t3);

	    tree t4 {tree_type::F_EACH};
	    t4.set_src_a (dst);
	    t4.set_dst (dst);
	    t2.push_child (t4);

	    if (true)
	      std::cerr << "converted: " << t2 << std::endl;

	    t = t2;
	  }
	unresolved.erase (dst);
      };

    auto isolated_subexpression = [&unresolved] (tree &ch)
      {
	// Sub-expressions of these nodes never resolve anything, but
	// can introduce unresolved slots.  E.g. in (child ?[count]),
	// the child resolves the count, but in (?[child] count), it
	// doesn't.
	auto unresolved2 = resolve_count (ch, {});
	unresolved.insert (unresolved2.begin (), unresolved2.end ());
      };

    switch (t.m_tt)
      {
      case tree_type::F_COUNT:
	{
	  slot_idx src = t.src_a ();
	  if (false)
	    {
	      std::cerr << "unresolved slot: " << src << std::endl;
	      std::cerr << "consumer: " << t << std::endl;
	    }

	  can_resolve ();
	  unresolved.insert (src);
	  return unresolved;
	}

      case tree_type::PRED_LAST:
	unresolved.insert (t.src_a ());
	return unresolved;

      case tree_type::FORMAT:
      case tree_type::CAPTURE:
	can_resolve ();
	// Fall through.
      case tree_type::CAT:
	for (auto it = t.m_children.rbegin (); it != t.m_children.rend (); ++it)
	  if (it->m_tt != tree_type::STR)
	    unresolved = resolve_count (*it, unresolved);
	return unresolved;

      case tree_type::PRED_NOT:
	return resolve_count (t.child (0), unresolved);

      case tree_type::ALT:
	{
	  // Each branch must resolve a slot if that slot should be
	  // resolved by ALT.  And resolves from each branch propagate
	  // further.  So return union of resolves of all branches.
	  std::set <slot_idx> ret;
	  for (auto &ch: t.m_children)
	    {
	      auto unresolved2 = resolve_count (ch, unresolved);
	      ret.insert (unresolved2.begin (), unresolved2.end ());
	    }
	  return ret;
	}

      case tree_type::PRED_AND:
      case tree_type::PRED_OR:
	isolated_subexpression (t.child (0));
	isolated_subexpression (t.child (1));
	return unresolved;

      case tree_type::ASSERT:
      case tree_type::PRED_SUBX_ALL:
      case tree_type::PRED_SUBX_ANY:
	isolated_subexpression (t.child (0));
	return unresolved;

      case tree_type::PROTECT:
	can_resolve ();
	isolated_subexpression (t.child (0));
	return unresolved;

      case tree_type::CLOSE_STAR:
	{
	  // Star's sub-expression may be applied not at all (in which
	  // case it doesn't resolve anything), once (it might resolve
	  // something, but might also introduce more unresolveds), or
	  // twice (where it might resolve its own unresolveds from
	  // the first iteration).

	  auto unresolved2 = resolve_count (t.child (0), unresolved);
	  unresolved.insert (unresolved2.begin (), unresolved2.end ());

	  auto unresolved3 = resolve_count (t.child (0), unresolved2);
	  unresolved.insert (unresolved3.begin (), unresolved3.end ());

	  return unresolved;
	}

      case tree_type::SHF_DROP:
	// Drop doesn't resolve anything even though it has a m_dst.
	// But in fact it's very suspicious if we are dropping a slot
	// that's later referenced by count, so check that that's not
	// the case.
	assert (unresolved.find (t.dst ()) == unresolved.end ());
	return unresolved;

      case tree_type::SHF_SWAP:
      case tree_type::SHF_DUP:
      case tree_type::SHF_OVER:
	{
	  // If these should resolve anything, they instead transfer
	  // the unresolvedness to their source slot.  They therefore
	  // don't resolve anything, as we need to generate the count
	  // somewhere else than at these nodes.  (E.g. in (child dup
	  // count) we want the child to annotate count, not the dup.)
	  auto it = unresolved.find (t.dst ());
	  if (it != unresolved.end ())
	    {
	      unresolved.erase (it);
	      unresolved.insert (t.src_a ());
	    }
	  return unresolved;
	}

      case tree_type::F_EACH:
	// If each resolves anything, we don't need to modify it in
	// any way, as it already takes care of annotating count.
	unresolved.erase (t.dst ());
	return unresolved;

      case tree_type::EMPTY_LIST:
      case tree_type::CONST:
      case tree_type::STR:
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
      case tree_type::F_LENGTH:
      case tree_type::SEL_UNIVERSE:
      case tree_type::SEL_WINFO:
      case tree_type::SEL_SECTION:
      case tree_type::SEL_UNIT:
	assert (t.m_children.empty ());
	can_resolve ();
	return unresolved;

      case tree_type::NOP:
      case tree_type::PRED_AT:
      case tree_type::PRED_TAG:
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
	return unresolved;

      case tree_type::TRANSFORM:
	return unresolved;

      case tree_type::SHF_ROT:
	assert (! "resolve_count: ROT unhandled");
	abort ();

      case tree_type::CLOSE_PLUS:
      case tree_type::MAYBE:
	assert (! "Should never get here.");
	abort ();
      }

    assert (t.m_tt != t.m_tt);
    abort ();
  }
}

void
tree::determine_stack_effects ()
{
  resolve_operands (*this, stack_refs {}, true).max ();

  // Count is potentially expensive and it would prevent most
  // producers from doing incremental work, even though count is
  // unlikely to be used often.  So instead we figure out which
  // producers produce the slot that count is applied to, and convert
  // the said producers X to ([X] each).  each is already written in
  // such a way as to annotate produced values with total count,
  // because each knows how long the sequence is.
  auto unresolved = resolve_count (*this, {});
  assert (unresolved.empty ());

  std::cerr << *this << std::endl;
}

void
tree::simplify ()
{
  // Recurse.
  for (auto &t: m_children)
    t.simplify ();

  // Promote CAT's in CAT nodes and ALT's in ALT nodes.  Parser does
  // this already, but other transformations may lead to re-emergence
  // of this pattern.
  if (m_tt == tree_type::CAT || m_tt == tree_type::ALT)
    while (true)
      {
	bool changed = false;
	for (size_t i = 0; i < m_children.size (); )
	  if (child (i).m_tt == m_tt)
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

  // Promote CAT's only child.
  if (m_tt == tree_type::CAT && m_children.size () == 1)
    {
      *this = child (0);
      simplify ();
    }

  // Change (FORMAT (STR)) to (STR).
  if (m_tt == tree_type::FORMAT
      && m_children.size () == 1
      && child (0).m_tt == tree_type::STR)
    {
      child (0).set_dst (dst ());
      *this = child (0);
      simplify ();
    }

  if (m_tt == tree_type::CAT)
    {
      // Drop NOP's in CAT nodes.
      auto it = std::remove_if (m_children.begin (), m_children.end (),
				[] (tree &t) {
				  return t.m_tt == tree_type::NOP;
				});
      if (it != m_children.end ())
	{
	  m_children.erase (it, m_children.end ());
	  simplify ();
	}
    }
}
