#include <iostream>
#include <cassert>
#include <set>

#include "patx.hh"
#include "dwpp.hh"

void
patx_group::append (std::shared_ptr <patx> expr)
{
  m_exprs.push_back (expr);
}

std::unique_ptr <wset>
patx_group::evaluate (std::unique_ptr <wset> &&ws)
{
  std::unique_ptr <wset> tmp (std::move (ws));
  for (auto &pat: m_exprs)
    tmp = pat->evaluate (std::move (tmp));

  return std::move (tmp);
}


patx_repeat::patx_repeat (std::shared_ptr <patx> patx, size_t min, size_t max)
  : m_patx (patx)
  , m_min (min)
  , m_max (max)
{}

std::unique_ptr <wset>
patx_repeat::evaluate (std::unique_ptr <wset> &&ws)
{
  // First, pass it through the minimum required number of iterations.
  for (size_t i = 0; i < m_min; ++i)
    ws = m_patx->evaluate (std::move (ws));

  // Next, we iterate until we either hit the upper bound, or until
  // the list of subgraphs in the working set stops growing (i.e. we
  // reached the fixed point).  To detect the latter, we store graphs
  // separately in a set.

  auto cmp = [] (std::unique_ptr <subgraph> const &a,
		 std::unique_ptr <subgraph> const &b)
    {
      std::vector <dieref> da {a->begin (), a->end ()};
      std::vector <dieref> db {b->begin (), b->end ()};
      return da < db;
    };

  std::set <std::unique_ptr <subgraph>, decltype (cmp)> sgs {cmp};
  std::vector <std::unique_ptr <value> > rest;

  auto split = [&sgs, &rest] (std::unique_ptr <wset> &&result)
    {
      for (auto &val: *result)
	if (auto sg = dynamic_cast <subgraph *> (&*val))
	  {
	    val.release ();
	    std::unique_ptr <subgraph> np (sg);
	    sgs.emplace (std::move (np));
	  }
	else
	  rest.emplace_back (std::move (val));
    };

  split (std::move (ws));

  size_t sz = sgs.size ();
  for (size_t i = m_min; i < m_max; ++i)
    {
      std::cout << "iteration " << i << ": " << sz << " elements\n";
      auto tmp = std::unique_ptr <wset> {new wset {}};
      for (auto &val: sgs)
	tmp->add (std::unique_ptr <subgraph> {new subgraph {*val}});

      split (m_patx->evaluate (std::move (tmp)));
      size_t sz2 = sgs.size ();
      if (sz == sz2)
	break;
      sz = sz2;
    }

  // Finally, merge into one wset again.
  auto ret = std::unique_ptr <wset> {new wset {}};
  for (auto &val: rest)
    ret->add (std::move (val));
  for (auto &val: sgs)
    {
      // val is constant in this context :-/
      auto tmp = std::unique_ptr <subgraph> (new subgraph {*val});
      ret->add (std::move (tmp));
    }

  return std::move (ret);
}


std::unique_ptr <wset>
patx_nodep::evaluate (std::unique_ptr <wset> &&ws)
{
  auto ret = std::unique_ptr <wset> {new wset {}};
  for (auto &val: *ws)
    if (match (*val))
      ret->add (std::move (val));
  return std::move (ret);
}


bool
patx_nodep_any::match (value const &val)
{
  return true;
}


patx_nodep_tag::patx_nodep_tag (int tag)
  : m_tag (tag)
{}

bool
patx_nodep_tag::match (value const &val)
{
  if (auto g = dynamic_cast <subgraph const *> (&val))
    {
      Dwarf_Die die = g->focus ();
      /*
      std::cout << "  ($tag == " << std::dec << m_tag << ") on "
		<< std::hex << dwarf_dieoffset (&die)
		<< ", which is " << std::dec << dwarf_tag (&die) << std::endl;
      */
      return dwarf_tag (&die) == m_tag;
    }
  else
    return false;
}


patx_nodep_hasattr::patx_nodep_hasattr (int attr)
  : m_attr (attr)
{}

bool
patx_nodep_hasattr::match (value const &val)
{
  if (auto g = dynamic_cast <subgraph const *> (&val))
    {
      Dwarf_Die die = g->focus ();
      return dwarf_hasattr_integrate (&die, m_attr);
    }
  else
    return false;
}


patx_nodep_not::patx_nodep_not (std::shared_ptr <patx_nodep> op)
  : m_op (op)
{}

bool
patx_nodep_not::match (value const &val)
{
  return ! m_op->match (val);
}


patx_nodep_and::patx_nodep_and (std::shared_ptr <patx_nodep> lhs,
				std::shared_ptr <patx_nodep> rhs)
  : m_lhs (lhs)
  , m_rhs (rhs)
{}

bool
patx_nodep_and::match (value const &val)
{
  return m_lhs->match (val) && m_rhs->match (val);
}


patx_nodep_or::patx_nodep_or (std::shared_ptr <patx_nodep> lhs,
			      std::shared_ptr <patx_nodep> rhs)
  : m_lhs (lhs)
  , m_rhs (rhs)
{}

bool
patx_nodep_or::match (value const &val)
{
  return m_lhs->match (val) || m_rhs->match (val);
}


std::unique_ptr <wset>
patx_edgep_child::evaluate (std::unique_ptr <wset> &&ws)
{
  auto ret = std::unique_ptr <wset> {new wset {}};
  for (auto &val: *ws)
    if (auto sg = dynamic_cast <subgraph const *> (&*val))
      {
	Dwarf_Die die = sg->focus ();
	if (dwarf_haschildren (&die))
	  {
	    dwpp_child (&die, &die);

	    do
	      {
		auto ng = std::unique_ptr <subgraph> {new subgraph {*sg}};
		ng->add (dieref (&die));
		ret->add (std::move (ng));
	      }
	    while (dwpp_siblingof (&die, &die));
	  }
      }
  return std::move (ret);
}


patx_edgep_attr::patx_edgep_attr (int attr)
  : m_attr (attr)
{}

std::unique_ptr <wset>
patx_edgep_attr::evaluate (std::unique_ptr <wset> &&ws)
{
  auto ret = std::unique_ptr <wset> {new wset {}};
  for (auto &val: *ws)
    if (auto sg = dynamic_cast <subgraph const *> (&*val))
      {
	Dwarf_Die die = sg->focus ();
	Dwarf_Attribute at;
	if (dwpp_attr_integrate (&die, m_attr, &at)
	    // Unfortunately we can't really distinguish whether
	    // dwarf_formref_die failed because it's not a reference
	    // (we would like to ignore those) or whether the
	    // underlying Dwarf is broken.  So just ignore it all.
	    && dwarf_formref_die (&at, &die) != NULL)
	  {
	    auto ng = std::unique_ptr <subgraph> {new subgraph {*sg}};
	    ng->add (dieref (&die));
	    ret->add (std::move (ng));
	  }
      }
  return std::move (ret);
}
