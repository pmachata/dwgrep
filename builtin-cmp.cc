#include <memory>
#include "make_unique.hh"
#include <iostream>

#include "builtin-cmp.hh"
#include "pred_result.hh"
#include "op.hh"

namespace
{
  pred_result
  comparison_result (valfile &vf, cmp_result want)
  {
    if (auto va = vf.get_as <value_cst> (0))
      if (auto vb = vf.get_as <value_cst> (1))
	// For two different domains, complain about comparisons that
	// don't have at least one comparand signed_constant_dom or
	// unsigned_constant_dom.
	if (va->get_constant ().dom () != vb->get_constant ().dom ()
	    && ! va->get_constant ().dom ()->safe_arith ()
	    && ! vb->get_constant ().dom ()->safe_arith ())
	  std::cerr << "Warning: comparing " << *va << " to " << *vb
		    << " is probably not meaningful (different domains).\n";

    auto &va = vf.get (0);
    auto &vb = vf.get (1);
    cmp_result r = vb.cmp (va);
    if (r == cmp_result::fail)
      {
	std::cerr << "Error: Can't compare `" << va << "' to `" << vb << "'\n.";
	return pred_result::fail;
      }
    else
      return pred_result (r == want);
  }

  struct pred_eq
    : public pred_binary
  {
    using pred_binary::pred_binary;

    pred_result
    result (valfile &vf) override
    {
      return comparison_result (vf, cmp_result::equal);
    }

    std::string
    name () const override
    {
      return "eq";
    }
  };

  struct pred_lt
    : public pred_binary
  {
    using pred_binary::pred_binary;

    pred_result
    result (valfile &vf) override
    {
      return comparison_result (vf, cmp_result::less);
    }

    std::string
    name () const override
    {
      return "lt";
    }
  };

  struct pred_gt
    : public pred_binary
  {
    using pred_binary::pred_binary;

    pred_result
    result (valfile &vf) override
    {
      return comparison_result (vf, cmp_result::greater);
    }

    std::string
    name () const override
    {
      return "gt";
    }
  };
}

std::unique_ptr <pred>
builtin_eq::build_pred (dwgrep_graph::sptr q,
			std::shared_ptr <scope> scope) const
{
  return maybe_invert (std::make_unique <pred_eq> ());
}

char const *
builtin_eq::name () const
{
  if (m_positive)
    return "?eq";
  else
    return "!eq";
}


std::unique_ptr <pred>
builtin_lt::build_pred (dwgrep_graph::sptr q,
			std::shared_ptr <scope> scope) const
{
  return maybe_invert (std::make_unique <pred_lt> ());
}

char const *
builtin_lt::name () const
{
  if (m_positive)
    return "?lt";
  else
    return "!lt";
}


std::unique_ptr <pred>
builtin_gt::build_pred (dwgrep_graph::sptr q,
			std::shared_ptr <scope> scope) const
{
  return maybe_invert (std::make_unique <pred_gt> ());
}

char const *
builtin_gt::name () const
{
  if (m_positive)
    return "?gt";
  else
    return "!gt";
}
