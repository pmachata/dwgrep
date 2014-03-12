#include <cassert>
#include <iostream>
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
tree::append_pipe (tree *t)
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
