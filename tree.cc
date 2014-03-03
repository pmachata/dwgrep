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
    case tree_arity_v::INT:
      m_u.ival = other.m_u.ival;
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
    case tree_arity_v::INT:
    case tree_arity_v::NULLARY:
    case tree_arity_v::UNARY:
    case tree_arity_v::BINARY:
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
tree::create_pipe (tree *t1, tree *t2)
{
  if (t1 == nullptr)
    {
      assert (t2 != nullptr);
      return t2;
    }
  else if (t2 == nullptr)
    return t1;
  else
    return tree::create_binary <tree_type::PIPE> (t1, t2);
}

void
tree::take_child (tree *t)
{
  m_children.push_back (*t);
  delete t;
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
    case tree_arity_v::INT:
      o << "<" << m_u.ival << ">";
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
