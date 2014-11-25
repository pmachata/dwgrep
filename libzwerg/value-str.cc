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

value_type const value_str::vtype = value_type::alloc ("T_STR",
R"docstring(

Values of this type hold strings (sequences of characters).  No
unicode support is provided as such, though UTF-8 should naturally
work.  Zwerg strings are not NUL-terminated::

	$ dwgrep '"abc \x00 def" (, length)'
	abc  def
	9

)docstring");

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


value_str
op_add_str::operate (std::unique_ptr <value_str> a,
		     std::unique_ptr <value_str> b)
{
  return value_str {a->get_string () + b->get_string (), 0};
}

std::string
op_add_str::docstring ()
{
  return
R"docstring(

``add`` concatenates two strings on TOS and yields the resulting string::

	$ dwgrep '"foo" "bar" add'
	foobar

Using formatting strings may be a better way to concatenate strings::

	$ dwgrep '"foo" "bar" "baz" "%s%s%s"'
	foobarbaz

)docstring";
}


value_cst
op_length_str::operate (std::unique_ptr <value_str> a)
{
  constant t {a->get_string ().length (), &dec_constant_dom};
  return value_cst {t, 0};
}

std::string
op_length_str::docstring ()
{
  return
R"docstring(

Yields length of string on TOS::

	dwgrep '"foo" length'
	3

)docstring";
}



namespace
{
  struct str_elem_producer_base
  {
    std::unique_ptr <value_str> m_v;
    size_t m_sz;
    char const *m_buf;
    size_t m_idx;

    str_elem_producer_base (std::unique_ptr <value_str> v)
      : m_v {std::move (v)}
      , m_sz {m_v->get_string ().size ()}
      , m_buf {m_v->get_string ().c_str ()}
      , m_idx {0}
    {}
  };

  struct str_elem_producer
    : public value_producer <value_str>
    , public str_elem_producer_base
  {
    using str_elem_producer_base::str_elem_producer_base;

    std::unique_ptr <value_str>
    next () override
    {
      if (m_idx < m_sz)
	{
	  char c = m_buf[m_idx];
	  return std::make_unique <value_str> (std::string {c}, m_idx++);
	}

      return nullptr;
    }
  };

  struct str_relem_producer
    : public value_producer <value_str>
    , public str_elem_producer_base
  {
    using str_elem_producer_base::str_elem_producer_base;

    std::unique_ptr <value_str>
    next () override
    {
      if (m_idx < m_sz)
	{
	  char c = m_buf[m_sz - 1 - m_idx];
	  return std::make_unique <value_str> (std::string {c}, m_idx++);
	}

      return nullptr;
    }
  };
}

namespace
{
  char const *const elem_str_docstring =
R"docstring(

The description at ``T_SEQ`` overload applies.  For strings, ``elem``
and ``relem`` yield individual characters of the string, again as
strings::

	$ dwgrep '["foo" elem]'
	[f, o, o]

)docstring";
}

// elem
std::unique_ptr <value_producer <value_str>>
op_elem_str::operate (std::unique_ptr <value_str> a)
{
  return std::make_unique <str_elem_producer> (std::move (a));
}

std::string
op_elem_str::docstring ()
{
  return elem_str_docstring;
}


// relem
std::unique_ptr <value_producer <value_str>>
op_relem_str::operate (std::unique_ptr <value_str> a)
{
  return std::make_unique <str_relem_producer> (std::move (a));
}

std::string
op_relem_str::docstring ()
{
  return elem_str_docstring;
}


// ?empty
pred_result
pred_empty_str::result (value_str &a)
{
  return pred_result (a.get_string () == "");
}

std::string
pred_empty_str::docstring ()
{
  return
R"docstring(

This predicate holds if the string on TOS is empty::

	$ dwgrep '"" ?empty ">%s< is empty"'
	>< is empty

	$ dwgrep '"\x00" !empty ">%s< is not empty"'
	>< is not empty

)docstring";
}


// ?find

extern char const g_find_docstring[];

pred_result
pred_find_str::result (value_str &haystack, value_str &needle)
{
  return pred_result (haystack.get_string ().find (needle.get_string ())
		      != std::string::npos);
}

std::string
pred_find_str::docstring ()
{
  return g_find_docstring;
}


// ?starts

extern char const g_starts_docstring[];

pred_result
pred_starts_str::result (value_str &haystack, value_str &needle)
{
  auto const &hay = haystack.get_string ();
  auto const &need = needle.get_string ();
  return pred_result
    (hay.size () >= need.size ()
     && hay.compare (0, need.size (), need) == 0);
}

std::string
pred_starts_str::docstring ()
{
  return g_starts_docstring;
}


// ?ends

extern char const g_ends_docstring[];

pred_result
pred_ends_str::result (value_str &haystack, value_str &needle)
{
  auto const &hay = haystack.get_string ();
  auto const &need = needle.get_string ();
  return pred_result
    (hay.size () >= need.size ()
     && hay.compare (hay.size () - need.size (), need.size (), need) == 0);
}

std::string
pred_ends_str::docstring ()
{
  return g_ends_docstring;
}


// ?match

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

std::string
pred_match_str::docstring ()
{
  return
R"docstring(

This asserts that TOS (which is a string with a regular expression)
matches the string below TOS.  The whole string has to match.  If you
want to look for matches anywhere in the string, just surround your
expression with ``.*``'s::

	"haystack" ?(".*needle.*" ?match)

For example::

	$ dwgrep '"foobar" "f.*r" ?match'
	---
	f.*r
	foobar

)docstring";
}
