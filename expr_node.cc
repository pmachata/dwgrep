#include "expr_node.hh"
#include "exec_node.hh"

#include <iostream>
#include <memory>
#include "make_unique.hh"

expr_node_cat::expr_node_cat (std::unique_ptr <expr_node> left,
			      std::unique_ptr <expr_node> right)
  : m_left (std::move (left))
  , m_right (std::move (right))
{}

std::pair <std::shared_ptr <exec_node>, std::shared_ptr <exec_node> >
expr_node_cat::build_exec (problem::ptr q)
{
  auto le = m_left->build_exec (q);
  auto re = m_right->build_exec (q);
  auto qe = std::make_shared <que_node> (le.second, re.first);
  le.second->assign_out (qe);
  re.first->assign_in (qe);
  return std::make_pair (le.first, re.second);
}

namespace
{
  struct exec_node_uni
    : public exec_node
  {
    void
    operate () override
    {
      while (valfile::ptr vf = next ())
	{
	  for (int i = 0; i < 250; ++i)
	    {
	      std::cout << "exec_node_uni::operate " << i << std::endl;
	      auto vf2 = std::make_unique <valfile> (*vf);
	      vf2->push_back (i);
	      push (std::move (vf2));
	    }
	}
    }

    std::string
    name () const override
    {
      return "uni";
    }
  };
}

std::pair <std::shared_ptr <exec_node>,
	   std::shared_ptr <exec_node> >
expr_node_uni::build_exec (problem::ptr q)
{
  return build_leaf_exec <exec_node_uni> ();
}

namespace
{
  struct exec_node_dup
    : public exec_node
  {
    void
    operate () override
    {
      while (valfile::ptr vf = next ())
	{
	  assert (! vf->empty ());
	  vf->push_back (vf->back ());
	  push (std::move (vf));
	}
    }

    std::string
    name () const override
    {
      return "dup";
    }
  };
}

std::pair <std::shared_ptr <exec_node>,
	   std::shared_ptr <exec_node> >
expr_node_dup::build_exec (problem::ptr q)
{
  return build_leaf_exec <exec_node_dup> ();
}

namespace
{
  struct exec_node_mul
    : public exec_node
  {
    void
    operate () override
    {
      while (valfile::ptr vf = next ())
	{
	  assert (vf->size () >= 2);
	  int a = vf->back (); vf->pop_back ();
	  int b = vf->back (); vf->pop_back ();
	  vf->push_back (a * b);
	  push (std::move (vf));
	}
    }

    std::string
    name () const override
    {
      return "mul";
    }
  };
}

std::pair <std::shared_ptr <exec_node>,
	   std::shared_ptr <exec_node> >
expr_node_mul::build_exec (problem::ptr q)
{
  return build_leaf_exec <exec_node_mul> ();
}
