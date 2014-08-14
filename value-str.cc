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

std::unique_ptr <value>
op_add_str::operate (std::unique_ptr <value_str> a,
		     std::unique_ptr <value_str> b)
{
  a->get_string () += b->get_string ();
  return std::move (a);
}

std::unique_ptr <value>
op_length_str::operate (std::unique_ptr <value_str> a)
{
  constant t {a->get_string ().length (), &dec_constant_dom};
  return std::make_unique <value_cst> (t, 0);
}

namespace
{
  struct str_elem_producer_base
  {
    std::unique_ptr <value_str> m_v;
    std::string const &m_str;
    size_t m_idx;

    str_elem_producer_base (std::unique_ptr <value_str> v)
      : m_v {std::move (v)}
      , m_str {m_v->get_string ()}
      , m_idx {0}
    {}
  };

  struct str_elem_producer
    : public value_producer
    , public str_elem_producer_base
  {
    using str_elem_producer_base::str_elem_producer_base;

    std::unique_ptr <value>
    next () override
    {
      if (m_idx < m_str.size ())
	{
	  char c = m_str[m_idx];
	  return std::make_unique <value_str> (std::string {c}, m_idx++);
	}

      return nullptr;
    }
  };

  struct str_relem_producer
    : public value_producer
    , public str_elem_producer_base
  {
    using str_elem_producer_base::str_elem_producer_base;

    std::unique_ptr <value>
    next () override
    {
      if (m_idx < m_str.size ())
	{
	  char c = m_str[m_str.size () - 1 - m_idx];
	  return std::make_unique <value_str> (std::string {c}, m_idx++);
	}

      return nullptr;
    }
  };
}

std::unique_ptr <value_producer>
op_elem_str::operate (std::unique_ptr <value_str> a)
{
  return std::make_unique <str_elem_producer> (std::move (a));
}

std::unique_ptr <value_producer>
op_relem_str::operate (std::unique_ptr <value_str> a)
{
  return std::make_unique <str_relem_producer> (std::move (a));
}

pred_result
pred_empty_str::result (value_str &a)
{
  return pred_result (a.get_string () == "");
}

pred_result
pred_find_str::result (value_str &haystack, value_str &needle)
{
  return pred_result (haystack.get_string ().find (needle.get_string ())
		      != std::string::npos);
}

pred_result
pred_match_str::result (value_str &haystack, value_str &needle)
{
  regex_t re;
  if (regcomp (&re, needle.get_string ().c_str(),
	       REG_EXTENDED | REG_NOSUB) != 0)
    {
      std::cerr << "Error: could not compile regular expression: '"
		<< needle.get_string () << "'\n";
      return pred_result::fail;
    }

  const int reti = regexec (&re, haystack.get_string ().c_str (),
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
      regerror (reti, &re, msgbuf, sizeof (msgbuf));
      std::cerr << "Error: match failed: " << msgbuf << "\n";
    }

  regfree (&re);
  return retval;
}
