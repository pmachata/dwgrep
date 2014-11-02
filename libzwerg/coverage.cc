/* Implementation of coverage analysis.

   Copyright (C) 2008, 2009, 2010 Red Hat, Inc.
   This file is part of Red Hat elfutils.

   Red Hat elfutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by the
   Free Software Foundation; version 2 of the License.

   Red Hat elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with Red Hat elfutils; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301 USA.

   Red Hat elfutils is an included package of the Open Invention Network.
   An included package of the Open Invention Network is a package for which
   Open Invention Network licensees cross-license their patents.  No patent
   license is granted, either expressly or impliedly, by designation as an
   included package.  Should you wish to participate in the Open Invention
   Network licensing program, please visit www.openinventionnetwork.com
   <http://www.openinventionnetwork.com>.  */

#include "coverage.hh"

#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <inttypes.h>
#include <elfutils/libdw.h>

#include "flag_saver.hh"

coverage::const_iterator
coverage::find (uint64_t start) const
{
  assert (!empty ());

  size_t a = 0;
  size_t b = size ();

  while (a < b)
    {
      size_t i = (a + b) / 2;
      cov_range const &r = at (i);

      if (r.start > start)
	b = i;
      else if (r.start < start)
	a = i + 1;
      else
	return begin () + i;
    }

  return begin () + a;
}

coverage::iterator
coverage::find (uint64_t start)
{
  const_iterator it = const_cast<coverage const *> (this)->find (start);
  return begin () + (it - begin ());
}

void
coverage::add (uint64_t start, uint64_t length)
{
  if (length == 0)
    return;

  cov_range nr = (struct cov_range){start, length};
  if (empty ())
    {
      push_back (nr);
      return;
    }

  iterator r_i = find (start);

  cov_range *to_insert = &nr;
  cov_range *coalesce = &nr;

  // Coalesce with previous range?
  if (r_i > begin ())
    {
      iterator p_i = r_i - 1;
      if (coalesce->start <= p_i->start + p_i->length)
	{
	  uint64_t coalesce_end = coalesce->start + coalesce->length;
	  if (coalesce_end > p_i->start + p_i->length)
	    {
	      p_i->length = coalesce_end - p_i->start;
	      coalesce = &*p_i;
	    }
	  else
	    coalesce = NULL;
	  to_insert = NULL;
	}
    }

  // Coalesce with one or more following ranges?
  if (coalesce != NULL && r_i != end ())
    {
      iterator p_i = r_i;
      while (p_i != end ()
	     && coalesce->start + coalesce->length >= p_i->start)
	{
	  uint64_t p_end = p_i->start + p_i->length;
	  if (p_end > coalesce->start + coalesce->length)
	    coalesce->length = p_end - coalesce->start;
	  if (to_insert != NULL)
	    {
	      *p_i = *to_insert;
	      to_insert = NULL;
	      coalesce = &*p_i;
	      assert (p_i == r_i);
	      ++r_i; // keep this element
	    }
	  ++p_i;
	}
      if (p_i > r_i)
	erase (r_i, p_i);
    }

  if (to_insert != NULL)
    {
      size_t idx = r_i - begin ();
      insert (begin () + idx, *to_insert);
    }
}

bool
coverage::remove (uint64_t start,
		  uint64_t length)
{
  if (empty () || length == 0)
    return false;

  uint64_t a_end = start + length;
  iterator r_i = find (start);
  iterator erase_begin_i = end ();
  iterator erase_end_i = r_i; // end exclusive
  bool overlap = false;

  // Cut from previous range?
  if (r_i > begin ())
    {
      iterator p_i = r_i - 1;
      if (start < p_i->start + p_i->length)
	{
	  uint64_t r_end = p_i->start + p_i->length;
	  // Do we cut the beginning of the range?
	  if (start == p_i->start)
	    p_i->length = a_end >= r_end ? 0 : r_end - a_end;
	  else
	    {
	      p_i->length = start - p_i->start;
	      // Do we shoot a hole in that range?
	      if (a_end < r_end)
		{
		  add (a_end, r_end - a_end);
		  return true;
		}
	    }

	  overlap = true;
	  if (p_i->length == 0)
	    erase_begin_i = p_i;
	}
    }

  if (erase_begin_i == end ())
    erase_begin_i = r_i;

  // Cut from next range?
  while (r_i < end () && r_i->start < a_end)
    {
      overlap = true;
      if (a_end >= r_i->start + r_i->length)
	{
	  ++erase_end_i;
	  ++r_i;
	}
      else
	{
	  uint64_t end0 = r_i->start + r_i->length;
	  r_i->length = end0 - a_end;
	  r_i->start = a_end;
	  assert (end0 == r_i->start + r_i->length);
	}
    }

  // Did we cut out anything completely?
  if (erase_end_i > erase_begin_i)
    erase (erase_begin_i, erase_end_i);

  return overlap;
}

bool
coverage::is_covered (uint64_t start, uint64_t length) const
{
  if (empty ())
    return false;

  const_iterator r_i = find (start);
  uint64_t a_end = start + length;
  if (r_i < end ())
    if (start >= r_i->start)
      return a_end <= r_i->start + r_i->length;

  if (r_i > begin ())
    {
      --r_i;
      return a_end <= r_i->start + r_i->length;
    }

  return false;
}

namespace pri
{
  class range
  {
    Dwarf_Off start;
    Dwarf_Off end;
  public:
    range (Dwarf_Off a_start, Dwarf_Off a_end)
      : start (a_start), end (a_end)
    {}
    friend std::ostream &operator << (std::ostream &os, range const &obj);
  };
  std::ostream &
  operator << (std::ostream &os, range const &obj)
  {
    ios_flag_saver fs {os};
    return os << "[" << std::hex << std::showbase << obj.start
	      << ", " << obj.end << ")";
  }
}

char *
range_fmt (char *buf, size_t buf_size, uint64_t start, uint64_t end)
{
  std::stringstream ss;
  ss << pri::range (start, end);
  std::string s = ss.str ();
  strncpy (buf, s.c_str (), buf_size);
  return buf;
}

namespace
{
  bool overlaps (uint64_t start, uint64_t end, cov_range const &r)
  {
    return (start >= r.start && start < r.start + r.length)
      || (end > r.start && end <= r.start + r.length)
      || (start < r.start && end > r.start + r.length);
  }
}

bool
coverage::is_overlap (uint64_t start, uint64_t length) const
{
  if (empty ())
    return false;
  if (length == 0)
    return is_covered (start, length);

  uint64_t a_end = start + length;
  const_iterator r_i = find (start);

  if (r_i < end () && overlaps (start, a_end, *r_i))
    return true;

  if (r_i > begin ())
    return overlaps (start, a_end, *--r_i);

  return false;
}

coverage
coverage::intersect (uint64_t start, uint64_t length) const
{
  if (empty () || length == 0)
    return coverage {};

  auto r_i = find (start);
  uint64_t a_end = start + length;

  coverage ret;

  // Handle intersection with previous range.
  if (r_i > begin ())
    {
      auto j = r_i - 1;
      if (start < j->start + j->length)
	ret.add (start, j->start + j->length - start);
    }

  // Handle intersection with following ranges.
  for (; r_i < end () && a_end > r_i->start; ++r_i)
    {
      uint64_t b_end = r_i->start + r_i->length;
      ret.add (r_i->start, std::min (b_end, a_end) - r_i->start);
    }

  return ret;
}

bool
coverage::find_holes (uint64_t start, uint64_t length,
		      bool (*hole)(uint64_t start, uint64_t length,
				   void *user_data),
		      void *user_data) const
{
  if (length == 0)
    return true;

  if (empty ())
    return hole (start, length, user_data);

  if (start < front ().start)
    if (!hole (start, front ().start - start, user_data))
      return false;

  for (size_t i = 0; i < size () - 1; ++i)
    {
      uint64_t end_i = at (i).end ();
      if (!hole (end_i, at (i+1).start - end_i, user_data))
	return false;
    }

  if (start + length > back ().end ())
    {
      uint64_t end_last = back ().end ();
      return hole (end_last, start + length - end_last, user_data);
    }

  return true;
}

bool
coverage::find_ranges (bool (*cb)(uint64_t start, uint64_t length, void *data),
		       void *user_data) const
{
  for (const_iterator it = begin (); it != end (); ++it)
    if (!cb (it->start, it->length, user_data))
      return false;

  return true;
}

void
coverage::add_all (coverage const &other)
{
  for (size_t i = 0; i < other.size (); ++i)
    add (other[i].start, other[i].length);
}

bool
coverage::remove_all (coverage const &other)
{
  bool ret = false;
  for (size_t i = 0; i < other.size (); ++i)
    if (remove (other[i].start, other[i].length))
      ret = true;
  return ret;
}

coverage
coverage::operator+ (coverage const &rhs) const
{
  coverage ret = *this;
  ret.add_all (rhs);
  return ret;
}

coverage
coverage::operator- (coverage const &rhs) const
{
  coverage ret = *this;
  ret.remove_all (rhs);
  return ret;
}


bool
cov::_format_base::fmt (uint64_t start, uint64_t length)
{
  if (_m_seen)
    _m_os << _m_delim;
  _m_os << pri::range (start, start + length);
  _m_seen = true;
  return true;
}

bool
cov::_format_base::wrap_fmt (uint64_t start, uint64_t length, void *data)
{
  _format_base *self = static_cast <_format_base *> (data);
  return self->fmt (start, length);
}

cov::_format_base::_format_base (std::string const &delim)
  : _m_delim (delim),
    _m_seen (false)
{}

cov::format_ranges::format_ranges (coverage const &cov,
				   std::string const &delim)
  : _format_base (delim)
{
  cov.find_ranges (&wrap_fmt, this);
}

cov::format_holes::format_holes (coverage const &cov,
				 uint64_t start, uint64_t length,
				 std::string const &delim)
  : _format_base (delim)
{
  cov.find_holes (start, length, &wrap_fmt, this);
}
