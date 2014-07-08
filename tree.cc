#include <algorithm>
#include <cassert>
#include <climits>
#include <iostream>
#include <memory>
#include <set>
#include <stdexcept>
#include <boost/optional.hpp>

#include "make_unique.hh"
#include "tree.hh"
#include "scope.hh"

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
  , m_scope {other.m_scope}
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

tree::tree (tree_type tt, std::shared_ptr <scope> scope)
  : m_scope {scope}
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
  std::swap (m_scope, other.m_scope);
  std::swap (m_tt, other.m_tt);
  std::swap (m_children, other.m_children);
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

std::shared_ptr <scope>
tree::scp () const
{
  return m_scope;
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

    case tree_arity_v::SCOPE:
      {
	auto scp = t.scp ();
	if (! scp->vars.empty ())
	  {
	    o << "{";
	    bool through = false;
	    for (auto const &s: scp->vars)
	      {
		if (through)
		  o << ";";
		through = true;
		o << s;
	      }
	    o << "}";
	  }
	break;
      }

    case tree_arity_v::NULLARY:
    case tree_arity_v::UNARY:
    case tree_arity_v::BINARY:
    case tree_arity_v::TERNARY:
      break;
    }

  for (auto const &child: t.m_children)
    o << " " << child;

  return o << ")";
}

bool
tree::operator< (tree const &that) const
{
  if (m_children.size () < that.m_children.size ())
    return true;
  else if (m_children.size () > that.m_children.size ())
    return false;

  auto it1 = m_children.begin ();
  auto it2 = that.m_children.begin ();
  for (; it1 != m_children.end (); ++it1, ++it2)
    if (*it1 < *it2)
      return true;
    else if (*it2 < *it1)
      return false;

  switch (argtype[(int) m_tt])
    {
    case tree_arity_v::CST:
      // Assume both are set.
      assert (m_cst != nullptr);
      assert (that.m_cst != nullptr);
      return *m_cst < *that.m_cst;

    case tree_arity_v::STR:
      // Assume both are set.
      assert (m_str != nullptr);
      assert (that.m_str != nullptr);
      return *m_str < *that.m_str;

    case tree_arity_v::SCOPE:
      // Two scopes simply always compare unequal, unless it's really
      // the same scope.
      return m_scope < that.m_scope;

    case tree_arity_v::NULLARY:
    case tree_arity_v::UNARY:
    case tree_arity_v::BINARY:
    case tree_arity_v::TERNARY:
      return false;
    }

  assert (! "Should never be reached.");
  abort ();
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
