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

#include <iostream>
#include <memory>
#include <regex.h>

#include "value-str.hh"
#include "overload.hh"
#include "value-cst.hh"

value_type const value_str::vtype = value_type::alloc ("T_STR");

void
value_str::show (std::ostream &o, brevity brv) const
{
  o << m_str;
}

std::unique_ptr <value>
value_str::clone () const
{
  return std::make_unique <value_str> (*this);
}

cmp_result
value_str::cmp (value const &that) const
{
  if (auto v = value::as <value_str> (&that))
    return compare (m_str, v->m_str);
  else
    return cmp_result::fail;
}

valfile::uptr
op_add_str::next ()
{
  if (auto vf = m_upstream->next ())
    {
      auto vp = vf->pop_as <value_str> ();
      auto wp = vf->pop_as <value_str> ();

      std::string result = wp->get_string () + vp->get_string ();
      vf->push (std::make_unique <value_str> (std::move (result), 0));
      return vf;
    }

  return nullptr;
}

valfile::uptr
op_length_str::next ()
{
  if (auto vf = m_upstream->next ())
    {
      auto vp = vf->pop_as <value_str> ();
      constant t {vp->get_string ().length (), &dec_constant_dom};
      vf->push (std::make_unique <value_cst> (t, 0));
      return vf;
    }

  return nullptr;
}

struct op_elem_str::state
{
  valfile::uptr m_base;
  std::string m_str;
  size_t m_idx;

  state (valfile::uptr base, std::string const &str)
    : m_base {std::move (base)}
    , m_str {str}
    , m_idx {0}
  {}

  valfile::uptr
  next ()
  {
    if (m_idx < m_str.size ())
      {
	char c = m_str[m_idx];
	auto v = std::make_unique <value_str> (std::string {c}, m_idx);
	valfile::uptr vf = std::make_unique <valfile> (*m_base);
	vf->push (std::move (v));
	m_idx++;
	return vf;
      }

    return nullptr;
  }
};

op_elem_str::op_elem_str (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
			  std::shared_ptr <scope> scope)
  : inner_op {upstream}
{}

op_elem_str::~op_elem_str ()
{}

valfile::uptr
op_elem_str::next ()
{
  while (true)
    {
      if (m_state == nullptr)
	{
	  if (auto vf = m_upstream->next ())
	    {
	      auto vp = vf->pop_as <value_str> ();
	      m_state = std::make_unique <state> (std::move (vf),
						  vp->get_string ());
	    }
	  else
	    return nullptr;
	}

      if (auto vf = m_state->next ())
	return vf;

      m_state = nullptr;
    }
}

std::string
op_elem_str::name () const
{
  return "elem_str";
}

void
op_elem_str::reset ()
{
  m_state = nullptr;
  inner_op::reset ();
}

pred_result
pred_empty_str::result (valfile &vf)
{
  auto vp = vf.top_as <value_str> ();
  return pred_result (vp->get_string () == "");
}

pred_result
pred_find_str::result (valfile &vf)
{
  auto needle = vf.get_as <value_str> (0);
  auto haystack = vf.get_as <value_str> (1);
  return pred_result (haystack->get_string ().find (needle->get_string ())
		      != std::string::npos);
}

pred_result
pred_match_str::result (valfile &vf)
{
  auto needle = vf.get_as <value_str> (0);
  auto haystack = vf.get_as <value_str> (1);

  regex_t re;
  if (regcomp (&re, needle->get_string ().c_str(),
	       REG_EXTENDED | REG_NOSUB) != 0)
    {
      std::cerr << "Error: could not compile regular expression: '"
		<< needle->get_string () << "'\n";
      return pred_result::fail;
    }

  const int reti = regexec (&re, haystack->get_string ().c_str (),
			    /* nmatch: size of pmatch array */ 0,
			    /* pmatch: array of matches */ NULL,
			    /* no extra flags */ 0);

  pred_result retval = pred_result::fail;
  if (reti == 0)
    retval = pred_result::yes;
  else if (reti == REG_NOMATCH)
    retval = pred_result::no;
  else
    {
      char msgbuf[100];
      regerror (reti, &re, msgbuf, sizeof(msgbuf));
      std::cerr << "Error: match failed: " << msgbuf << "\n";
    }

  regfree (&re);
  return retval;
}
