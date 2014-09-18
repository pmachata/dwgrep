/* Coverage analysis, C++ support.

   Copyright (C) 2008, 2009, 2010, 2014 Red Hat, Inc.
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

#ifndef DWARFLINT_COVERAGE_HH
#define DWARFLINT_COVERAGE_HH

#include <string>
#include <sstream>
#include <vector>
#include <cstdint>

/* Functions and data structures for handling of address range
   coverage.  We use that to find holes of unused bytes in DWARF
   string table.  */

struct cov_range
{
  uint64_t start;
  uint64_t length;

  uint64_t end () const { return start + length; }

  bool operator== (cov_range const &rhs) const
  {
    return start == rhs.start
      && length == rhs.length;
  }
};

struct coverage
  : private std::vector<cov_range>
{
  iterator find (uint64_t start);
  const_iterator find (uint64_t start) const;

public:
  using std::vector <cov_range>::size;
  using std::vector <cov_range>::empty;
  using std::vector <cov_range>::at;

  void add (uint64_t start, uint64_t length);

  /// Returns true if something was actually removed, false if whole
  /// range falls into hole in coverage.
  bool remove (uint64_t start, uint64_t length);

  void add_all (coverage const &other);

  // Returns true if something was actually removed, false if whole
  // range falls into hole in coverage.
  bool remove_all (coverage const &other);

  bool find_ranges (bool (*cb)(uint64_t start, uint64_t length, void *data),
		    void *data) const;

  /// Returns true if whole range ADDRESS/LENGTH is covered by COV.
  /// If LENGTH is zero, it's checked that the address is inside or at
  /// the edge of covered range, or that there is a zero-length range
  /// at that address.
  bool is_covered (uint64_t start, uint64_t length) const;

  /// Returns true if at least some of the range ADDRESS/LENGTH is
  /// covered by COV.  Zero-LENGTH range never overlaps.  */
  bool is_overlap (uint64_t start, uint64_t length) const;

  /// Intersect a range with this coverage.  Returns a new coverage
  /// object that is result of trimming START and LENGTH such that a)
  /// the new values form a (generally non-strict) subset of the
  /// original values, and b) all addresses in the returned coverage
  /// are covered by this coverage.  Returns an empty coverage if
  /// START/LENGTH don't overlap with this coverage at all.
  coverage intersect (uint64_t start, uint64_t length) const;

  bool find_holes (uint64_t start, uint64_t length,
		   bool (*cb)(uint64_t start, uint64_t length, void *data),
		   void *data) const;

  coverage operator+ (coverage const &rhs) const;
  coverage operator- (coverage const &rhs) const;
  bool operator== (coverage const &rhs) const
  {
    return static_cast<std::vector<cov_range> > (rhs) == *this;
  }
};

char *range_fmt (char *buf, size_t buf_size,
		 uint64_t start, uint64_t end);

namespace cov
{
  class _format_base
  {
  protected:
    std::string const &_m_delim;
    std::ostringstream _m_os;
    bool _m_seen;

    inline bool fmt (uint64_t start, uint64_t length);
    static bool wrap_fmt (uint64_t start, uint64_t length, void *data);
    _format_base (std::string const &delim);

  public:
    inline operator std::string () const
    {
      return _m_os.str ();
    }
  };

  struct format_ranges
    : public _format_base
  {
    format_ranges (coverage const &cov, std::string const &delim = ", ");
  };

  struct format_holes
    : public _format_base
  {
    format_holes (coverage const &cov, uint64_t start, uint64_t length,
		  std::string const &delim = ", ");
  };
}

inline std::ostream &
operator << (std::ostream &os, cov::format_ranges const &obj)
{
  return os << std::string (obj);
}

inline std::ostream &
operator << (std::ostream &os, cov::format_holes const &obj)
{
  return os << std::string (obj);
}
#endif//DWARFLINT_COVERAGE_HH
