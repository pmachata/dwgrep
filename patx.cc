#include <iostream>
#include <cassert>

#include "patx.hh"
#include "dwpp.hh"

void
patx_group::append (std::shared_ptr <patx> expr)
{
  m_exprs.push_back (expr);
}

std::unique_ptr <wset>
patx_group::evaluate (std::unique_ptr <wset> &ws)
{
  std::unique_ptr <wset> tmp (std::move (ws));
  for (auto &pat: m_exprs)
    tmp = pat->evaluate (tmp);

  return std::move (tmp);
}


std::unique_ptr <wset>
patx_nodep::evaluate (std::unique_ptr <wset> &ws)
{
  auto ret = std::unique_ptr <wset> (new wset ());
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
patx_edgep_child::evaluate (std::unique_ptr <wset> &ws)
{
  auto ret = std::unique_ptr <wset> (new wset ());
  for (auto &val: *ws)
    if (auto sg = dynamic_cast <subgraph const *> (&*val))
      {
	Dwarf_Die die = sg->focus ();
	if (dwarf_haschildren (&die))
	  {
	    dwpp_child (&die, &die);

	    do
	      {
		auto ng = std::unique_ptr <subgraph> (new subgraph (*sg));
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
patx_edgep_attr::evaluate (std::unique_ptr <wset> &ws)
{
  auto ret = std::unique_ptr <wset> (new wset ());
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
	    auto ng = std::unique_ptr <subgraph> (new subgraph (*sg));
	    ng->add (dieref (&die));
	    ret->add (std::move (ng));
	  }
      }
  return std::move (ret);
}
