#include "expr_node.hh"
#include "exec_node.hh"

#include <iostream>
#include <memory>
#include <cassert>
#include <algorithm>
#include "make_unique.hh"

expr_node_cat::expr_node_cat (std::unique_ptr <expr_node> left,
			      std::unique_ptr <expr_node> right)
  : m_left (std::move (left))
  , m_right (std::move (right))
{}

std::shared_ptr <exec_node>
expr_node_cat::build_exec (std::shared_ptr <exec_node> upstream,
			   dwgrep_graph::ptr q)
{
  return m_right->build_exec (m_left->build_exec (upstream, q), q);
}

namespace
{
  // Tine is placed at the beginning of each alt expression.  These
  // tines together share a common file, which next() takes data from
  // (each vector element belongs to one tine of the overall alt).
  //
  // Tine yields null if the file slot corresponding to its branch has
  // already been fetched.  It won't refill the file unless all other
  // tines have fetched as well.  Tine and merge need to cooperate to
  // make sure nullptr's don't get propagated unless there's really
  // nothing left.  Merge knows that the source is drained, if all its
  // branches yield nullptr.
  class exec_node_tine
    : public exec_node_unary
  {
    std::shared_ptr <std::vector <valfile::ptr> > m_file;
    size_t m_branch_id;

  public:
    exec_node_tine (std::shared_ptr <exec_node> exec,
		    std::shared_ptr <std::vector <valfile::ptr> > file,
		    size_t branch_id)
      : exec_node_unary (exec)
      , m_file (file)
      , m_branch_id (branch_id)
    {
      assert (m_branch_id < m_file->size ());
    }

    valfile::ptr
    next () override
    {
      if (std::all_of (m_file->begin (), m_file->end (),
		       [] (valfile::ptr const &ptr) { return ptr == nullptr; }))
	{
	  if (auto vf = m_exec->next ())
	    for (auto &ptr: *m_file)
	      ptr = std::make_unique <valfile> (*vf);
	  else
	    return nullptr;
	}

      return std::move ((*m_file)[m_branch_id]);
    }

    std::string
    name () const override
    {
      return "tine";
    }
  };

  class exec_node_merge
    : public exec_node
  {
    typedef std::vector <std::shared_ptr <exec_node> > execvect_t;
    execvect_t m_execs;
    execvect_t::iterator m_it;

  public:
    exec_node_merge (std::vector <std::shared_ptr <exec_node> > execs)
      : m_execs (execs)
      , m_it (m_execs.begin ())
    {}

    valfile::ptr
    next () override
    {
      if (m_it == m_execs.end ())
	return nullptr;

      for (size_t i = 0; i < m_execs.size (); ++i)
	{
	  if (auto ret = (*m_it)->next ())
	    return ret;
	  if (++m_it == m_execs.end ())
	    m_it = m_execs.begin ();
	}
      m_it = m_execs.end ();
      return nullptr;
    }

    std::string
    name () const override
    {
      return "merge";
    }
  };
}

expr_node_alt::expr_node_alt
	(std::vector <std::unique_ptr <expr_node> > branches)
	  : m_branches (std::move (branches))
{}

std::shared_ptr <exec_node>
expr_node_alt::build_exec (std::shared_ptr <exec_node> upstream,
			   dwgrep_graph::ptr q)
{
  std::vector <std::shared_ptr <exec_node> > execs;

  {
    auto f = std::make_shared <std::vector <valfile::ptr> >
      (m_branches.size ());
    for (size_t i = 0; i < m_branches.size (); ++i)
      execs.push_back (std::make_shared <exec_node_tine> (upstream, f, i));
  }

  auto build_branch = [&q] (std::unique_ptr <expr_node> &br,
			    std::shared_ptr <exec_node> &ex)
    {
      return br->build_exec (ex, q);
    };

  std::transform (m_branches.begin (), m_branches.end (),
		  execs.begin (), execs.begin (), build_branch);

  return std::make_shared <exec_node_merge> (execs);
}

namespace
{
  class exec_node_query
    : public exec_node
  {
    bool m_done;
  public:
    exec_node_query ()
      : m_done (false)
    {}

    valfile::ptr
    next () override
    {
      if (m_done)
	return nullptr;
      m_done = true;
      return std::make_unique <valfile> ();
    }

    std::string
    name () const override
    {
      return "query";
    }
  };
}

query_expr::query_expr (std::unique_ptr <expr_node> expr)
  : m_expr (std::move (expr))
{}

std::shared_ptr <exec_node>
query_expr::build_exec (dwgrep_graph::ptr q)
{
  return m_expr->build_exec (std::make_shared <exec_node_query> (), q);
}

namespace
{
  struct exec_node_uni
    : public exec_node_unary
  {
    valfile::ptr m_vf;
    int i;

  public:
    exec_node_uni (std::shared_ptr <exec_node> exec)
      : exec_node_unary (exec)
      , i (0)
    {}

    valfile::ptr
    next () override
    {
      while (true)
	{
	  if (m_vf == nullptr)
	    m_vf = m_exec->next ();
	  if (m_vf == nullptr)
	    return nullptr;
	  if (i < 50000000)
	    {
	      auto ret = std::make_unique <valfile> (*m_vf);
	      ret->push_back (i++);
	      return ret;
	    }
	  i = 0;
	  m_vf = nullptr;
	}
    }

    std::string
    name () const override
    {
      return "uni";
    }
  };
}

std::shared_ptr <exec_node>
expr_node_uni::build_exec (std::shared_ptr <exec_node> upstream,
			   dwgrep_graph::ptr q)
{
  return std::make_shared <exec_node_uni> (upstream);
}

namespace
{
  struct exec_node_dup
    : public exec_node_unary
  {
    using exec_node_unary::exec_node_unary;

    valfile::ptr
    next () override
    {
      if (auto vf = m_exec->next ())
	{
	  assert (vf->size () > 0);
	  vf->push_back (vf->back ());
	  return vf;
	}
      return nullptr;
    }

    std::string
    name () const override
    {
      return "dup";
    }
  };
}

std::shared_ptr <exec_node>
expr_node_dup::build_exec (std::shared_ptr <exec_node> upstream,
			   dwgrep_graph::ptr q)
{
  return std::make_shared <exec_node_dup> (upstream);
}

namespace
{
  struct exec_node_mul
    : public exec_node_unary
  {
    using exec_node_unary::exec_node_unary;

    valfile::ptr
    next () override
    {
      if (auto vf = m_exec->next ())
	{
	  assert (vf->size () >= 2);
	  int a = vf->back (); vf->pop_back ();
	  int b = vf->back (); vf->pop_back ();
	  vf->push_back (a * b);
	  return vf;
	}
      return nullptr;
    }

    std::string
    name () const override
    {
      return "mul";
    }
  };
}

std::shared_ptr <exec_node>
expr_node_mul::build_exec (std::shared_ptr <exec_node> upstream,
			   dwgrep_graph::ptr q)
{
  return std::make_shared <exec_node_mul> (upstream);
}

namespace
{
  struct exec_node_add
    : public exec_node_unary
  {
    using exec_node_unary::exec_node_unary;

    valfile::ptr
    next () override
    {
      if (auto vf = m_exec->next ())
	{
	  assert (vf->size () >= 2);
	  int a = vf->back (); vf->pop_back ();
	  int b = vf->back (); vf->pop_back ();
	  vf->push_back (a + b);
	  return vf;
	}
      return nullptr;
    }

    std::string
    name () const override
    {
      return "add";
    }
  };
}

std::shared_ptr <exec_node>
expr_node_add::build_exec (std::shared_ptr <exec_node> upstream,
			   dwgrep_graph::ptr q)
{
  return std::make_shared <exec_node_add> (upstream);
}

namespace
{
  struct exec_node_odd
    : public exec_node_unary
  {
    using exec_node_unary::exec_node_unary;

    valfile::ptr
    next () override
    {
      while (auto vf = m_exec->next ())
	if (vf->back () % 2 != 0)
	  return vf;
      return nullptr;
    }

    std::string
    name () const override
    {
      return "odd";
    }
  };
}

std::shared_ptr <exec_node>
expr_node_odd::build_exec (std::shared_ptr <exec_node> upstream,
			   dwgrep_graph::ptr q)
{
  return std::make_shared <exec_node_odd> (upstream);
}

namespace
{
  struct exec_node_even
    : public exec_node_unary
  {
    using exec_node_unary::exec_node_unary;

    valfile::ptr
    next () override
    {
      while (auto vf = m_exec->next ())
	if (vf->back () % 2 == 0)
	  return vf;
      return nullptr;
    }

    std::string
    name () const override
    {
      return "even";
    }
  };
}

std::shared_ptr <exec_node>
expr_node_even::build_exec (std::shared_ptr <exec_node> upstream,
			   dwgrep_graph::ptr q)
{
  return std::make_shared <exec_node_even> (upstream);
}
