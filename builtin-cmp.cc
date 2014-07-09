#include <iostream>

#include "builtin.hh"
#include "pred_result.hh"
#include "op.hh"

namespace
{
  pred_result
  comparison_result (valfile &vf, cmp_result want)
  {
    if (auto va = vf.top_as <value_cst> ())
      if (auto vb = vf.below_as <value_cst> ())
	// For two different domains, complain about comparisons that
	// don't have at least one comparand signed_constant_dom or
	// unsigned_constant_dom.
	if (va->get_constant ().dom () != vb->get_constant ().dom ()
	    && ! va->get_constant ().dom ()->safe_arith ()
	    && ! vb->get_constant ().dom ()->safe_arith ())
	  std::cerr << "Warning: comparing " << *va << " to " << *vb
		    << " is probably not meaningful (different domains).\n";

    auto &va = vf.top ();
    auto &vb = vf.below ();
    cmp_result r = vb.cmp (va);
    if (r == cmp_result::fail)
      {
	std::cerr << "Error: Can't compare `" << va << "' to `" << vb << "'\n.";
	return pred_result::fail;
      }
    else
      return pred_result (r == want);
  }
}

static struct
  : public pred_builtin
{
  using pred_builtin::pred_builtin;

  struct eq
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

  std::unique_ptr <pred>
  build_pred (dwgrep_graph::sptr q,
	      std::shared_ptr <scope> scope) const override
  {
    return maybe_invert (std::make_unique <eq> ());
  }

  char const *
  name () const
  {
    if (m_positive)
      return "?eq";
    else
      return "!eq";
  }
} builtin_eq_obj {true}, builtin_neq_obj {false};


static struct
  : public pred_builtin
{
  using pred_builtin::pred_builtin;

  struct lt
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

  std::unique_ptr <pred>
  build_pred (dwgrep_graph::sptr q,
	      std::shared_ptr <scope> scope) const override
  {
    return maybe_invert (std::make_unique <lt> ());
  }

  char const *
  name () const
  {
    if (m_positive)
      return "?lt";
    else
      return "!lt";
  }
} builtin_lt_obj {true}, builtin_nlt_obj {false};


static struct
  : public pred_builtin
{
  using pred_builtin::pred_builtin;

  struct gt
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

  std::unique_ptr <pred>
  build_pred (dwgrep_graph::sptr q,
	      std::shared_ptr <scope> scope) const override
  {
    return maybe_invert (std::make_unique <gt> ());
  }

  char const *
  name () const
  {
    if (m_positive)
      return "?gt";
    else
      return "!gt";
  }
} builtin_gt_obj {true}, builtin_ngt_obj {false};

static struct register_cmp
{
  register_cmp ()
  {
    add_builtin (builtin_eq_obj);
    add_builtin (builtin_neq_obj);
    add_builtin (builtin_lt_obj);
    add_builtin (builtin_nlt_obj);
    add_builtin (builtin_gt_obj);
    add_builtin (builtin_ngt_obj);

    add_builtin (builtin_neq_obj, "?ne");
    add_builtin (builtin_eq_obj, "!ne");
    add_builtin (builtin_nlt_obj, "?ge");
    add_builtin (builtin_lt_obj, "!ge");
    add_builtin (builtin_ngt_obj, "?le");
    add_builtin (builtin_gt_obj, "!le");
  }
} register_cmp;
