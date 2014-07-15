#include <iostream>
#include <memory>
#include "make_unique.hh"

#include "value-closure.hh"
#include "tree.hh"

value_type const value_closure::vtype = value_type::alloc ("T_CLOSURE");

value_closure::value_closure (tree const &t, dwgrep_graph::sptr q,
			      std::shared_ptr <scope> scope,
			      std::shared_ptr <frame> frame, size_t pos)
  : value {vtype, pos}
  , m_t {std::make_unique <tree> (t)}
  , m_q {q}
  , m_scope {scope}
  , m_frame {frame}
{}

value_closure::value_closure (value_closure const &that)
  : value_closure {*that.m_t, that.m_q, that.m_scope, that.m_frame,
		   that.get_pos ()}
{}

value_closure::~value_closure()
{}

void
value_closure::show (std::ostream &o) const
{
  o << "closure(" << *m_t << ")";
}

std::unique_ptr <value>
value_closure::clone () const
{
  return std::make_unique <value_closure> (*this);
}

cmp_result
value_closure::cmp (value const &v) const
{
  if (auto that = value::as <value_closure> (&v))
    {
      auto a = std::make_tuple (static_cast <tree &> (*m_t), m_scope, m_frame);
      auto b = std::make_tuple (static_cast <tree &> (*that->m_t),
				that->m_scope, that->m_frame);
      return compare (a, b);
    }
  else
    return cmp_result::fail;
}
