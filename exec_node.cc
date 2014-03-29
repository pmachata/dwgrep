#include <system_error>
#include <iostream>
#include <memory>
#include <cassert>
#include "make_unique.hh"

#include "exec_node.hh"
#include "expr_node.hh"
#include "dwgrep.hh"

namespace
{
  inline void
  throw_system ()
  {
    throw std::runtime_error
      (std::error_code (errno, std::system_category ()).message ());
  }
}


namespace
{
  // This function returns what a parser would.
  std::unique_ptr <expr_node>
  build_my_expr (std::string const &)
  {
    auto A = std::make_unique <expr_node_cat>
      (std::make_unique <expr_node_odd> (),
       std::make_unique <expr_node_mul> ());
    auto B = std::make_unique <expr_node_cat>
      (std::make_unique <expr_node_even> (),
       std::make_unique <expr_node_add> ());

    std::vector <std::unique_ptr <expr_node> > alts;
    alts.push_back (std::move (A));
    alts.push_back (std::move (B));
    auto Alt = std::make_unique <expr_node_alt> (std::move (alts));

    return std::make_unique <expr_node_cat>
      (std::make_unique <expr_node_uni> (),
       std::make_unique <expr_node_cat>
       (std::make_unique <expr_node_dup> (), std::move (Alt)));
  }

  std::unique_ptr <query_expr>
  wrap_expr (std::unique_ptr <expr_node> expr)
  {
    return std::make_unique <query_expr> (std::move (expr));
  }
}

struct dwgrep_expr::pimpl
{
  std::unique_ptr <query_expr> m_expr;

  explicit pimpl (std::string const &str)
    : m_expr (wrap_expr (build_my_expr (str)))
  {}
};

struct dwgrep_expr::result::pimpl
{
  std::shared_ptr <exec_node> m_exec;
  explicit pimpl (std::shared_ptr <exec_node> exec)
    : m_exec (exec)
  {}
};

struct dwgrep_expr::result::iterator::pimpl
{
  std::shared_ptr <exec_node> m_exec;
  std::shared_ptr <std::vector <int> > m_vf;

  pimpl () {}

  pimpl (std::shared_ptr <exec_node> exec)
    : m_exec ((assert (exec != nullptr), exec))
  {
    fetch ();
  }

  pimpl (pimpl const &other) = default;

  bool
  done () const
  {
    return m_exec == nullptr;
  }

  void
  fetch ()
  {
    assert (! done ());
    m_vf = nullptr;

    auto res = m_exec->next ();
    if (res == nullptr)
      {
	m_exec = nullptr;
	return;
      }

    m_vf = std::make_shared <std::vector <int> > ();
    for (auto const &nv: *res)
      m_vf->push_back (nv);
  }

  bool
  equal (pimpl const &that) const
  {
    if (that.m_exec == nullptr)
      return m_exec == nullptr;
    return that.m_exec == m_exec
      && that.m_vf == m_vf;
  }
};

dwgrep_expr::dwgrep_expr (std::string const &str)
  : m_pimpl (std::make_unique <pimpl> (str))
{}

dwgrep_expr::~dwgrep_expr ()
{}

dwgrep_expr::result
dwgrep_expr::query (std::shared_ptr <dwgrep_graph> p)
{
  auto exec = m_pimpl->m_expr->build_exec (p);
  return result (std::make_unique <result::pimpl> (exec));
}

dwgrep_expr::result
dwgrep_expr::query (dwgrep_graph p)
{
  return query (std::make_shared <dwgrep_graph> (p));
}

dwgrep_expr::result::result (std::unique_ptr <pimpl> p)
  : m_pimpl (std::move (p))
{}

dwgrep_expr::result::~result ()
{}

dwgrep_expr::result::result (result &&that)
  : m_pimpl (std::move (that.m_pimpl))
{}

dwgrep_expr::result::iterator
dwgrep_expr::result::begin ()
{
  return iterator (std::make_unique <iterator::pimpl> (m_pimpl->m_exec));
}

dwgrep_expr::result::iterator
dwgrep_expr::result::end ()
{
  return iterator ();
}

dwgrep_expr::result::iterator::iterator ()
  : m_pimpl (std::make_unique <pimpl> ())
{}

dwgrep_expr::result::iterator::~iterator ()
{}

dwgrep_expr::result::iterator::iterator (iterator const &that)
  : m_pimpl (std::make_unique <pimpl> (*that.m_pimpl))
{}

dwgrep_expr::result::iterator::iterator (std::unique_ptr <pimpl> p)
  : m_pimpl (std::move (p))
{}

std::vector <int>
dwgrep_expr::result::iterator::operator* () const
{
  return *m_pimpl->m_vf;
}

dwgrep_expr::result::iterator
dwgrep_expr::result::iterator::operator++ ()
{
  m_pimpl->fetch ();
  return *this;
}

dwgrep_expr::result::iterator
dwgrep_expr::result::iterator::operator++ (int)
{
  iterator ret = *this;
  ++*this;
  return ret;
}

bool
dwgrep_expr::result::iterator::operator== (iterator that)
{
  return m_pimpl->equal (*that.m_pimpl);
}

bool
dwgrep_expr::result::iterator::operator!= (iterator that)
{
  return !(*this == that);
}
