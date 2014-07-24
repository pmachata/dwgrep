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
#include "make_unique.hh"

#include "value-cst.hh"

value_type const value_cst::vtype = value_type::alloc ("T_CONST");

void
value_cst::show (std::ostream &o, bool full) const
{
  o << m_cst;
}

std::unique_ptr <value>
value_cst::clone () const
{
  return std::make_unique <value_cst> (*this);
}

cmp_result
value_cst::cmp (value const &that) const
{
  if (auto v = value::as <value_cst> (&that))
    {
      // We don't want to evaluate as equal two constants from
      // different domains just because they happen to have the same
      // value.
      if (! m_cst.dom ()->safe_arith () || ! v->m_cst.dom ()->safe_arith ())
	{
	  cmp_result ret = compare (m_cst.dom (), v->m_cst.dom ());
	  if (ret != cmp_result::equal)
	    return ret;
	}

      // Either they are both arithmetic, or they are both from the
      // same non-arithmetic domain.  We can directly compare the
      // values now.
      return compare (m_cst, v->m_cst);
    }
  else
    return cmp_result::fail;
}

valfile::uptr
op_add_cst::next ()
{
  if (auto vf = m_upstream->next ())
    {
      auto vp = vf->pop_as <value_cst> ();
      auto wp = vf->pop_as <value_cst> ();

      constant const &cst_v = vp->get_constant ();
      constant const &cst_w = wp->get_constant ();
      check_arith (cst_v, cst_w);

      constant_dom const *dom = cst_v.dom ()->plain ()
	? cst_w.dom () : cst_v.dom ();

      constant result {cst_v.value () + cst_w.value (), dom};
      vf->push (std::make_unique <value_cst> (result, 0));
      return vf;
    }

  return nullptr;
}

valfile::uptr
op_value_cst::next ()
{
  if (auto vf = m_upstream->next ())
    {
      auto vp = vf->pop_as <value_cst> ();
      constant cst {vp->get_constant ().value (), &dec_constant_dom};
      vf->push (std::make_unique <value_cst> (cst, 0));
      return vf;
    }

  return nullptr;
}
