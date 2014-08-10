/*
   Copyright (C) 2014 Red Hat, Inc.
   This file is part of dwgrep.

   This file is free software; you can redistribute it and/or modify
   it under the terms of either

     * the GNU Lesser General Public License as published by the Free
       Software Foundation; either version 3 of the License, or (at
       your option) any later version

   or

     * the GNU General Public License as published by the Free
       Software Foundation; either version 2 of the License, or (at
       your option) any later version

   or both in parallel, as here.

   dwgrep is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received copies of the GNU General Public License and
   the GNU Lesser General Public License along with this program.  If
   not, see <http://www.gnu.org/licenses/>.  */

#include <memory>
#include <iostream>

#include "builtin-cmp.hh"
#include "op.hh"
#include "pred_result.hh"
#include "value-cst.hh"

namespace
{
  pred_result
  comparison_result (stack &stk, cmp_result want)
  {
    if (auto va = stk.get_as <value_cst> (0))
      if (auto vb = stk.get_as <value_cst> (1))
	check_constants_comparable (va->get_constant (), vb->get_constant ());

    auto &va = stk.get (0);
    auto &vb = stk.get (1);
    cmp_result r = vb.cmp (va);
    if (r == cmp_result::fail)
      {
	std::cerr << "Error: Can't compare `" << va << "' to `" << vb << "'.\n";
	return pred_result::fail;
      }
    else
      return pred_result (r == want);
  }

  struct pred_eq
    : public pred
  {
    pred_result
    result (stack &stk) override
    {
      return comparison_result (stk, cmp_result::equal);
    }

    std::string
    name () const override
    {
      return "eq";
    }

    void reset () override {}
  };

  struct pred_lt
    : public pred
  {
    pred_result
    result (stack &stk) override
    {
      return comparison_result (stk, cmp_result::less);
    }

    std::string
    name () const override
    {
      return "lt";
    }

    void reset () override {}
  };

  struct pred_gt
    : public pred
  {
    pred_result
    result (stack &stk) override
    {
      return comparison_result (stk, cmp_result::greater);
    }

    std::string
    name () const override
    {
      return "gt";
    }

    void reset () override {}
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
