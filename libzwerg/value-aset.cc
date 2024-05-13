/*
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

#include "value-aset.hh"

value_type const value_aset::vtype = value_type::alloc ("T_ASET",
R"docstring(

Values of this type contain sets of addresses.  They are used for
representing address ranges of all sorts.  They behave a bit like
sequences of constants, but calling ``elem`` is not advised unless you
can be sure that the address range is not excessively large::

	$ dwgrep ./tests/bitcount.o -e 'entry @AT_location address'
	[0x10000, 0x10017)
	[0x10017, 0x1001a)
	[0x1001a, 0x10020)
	[0x10000, 0x10007)
	[0x10007, 0x1001e)
	[0x1001e, 0x10020)

Address sets don't have to be continuous.

)docstring");

void
value_aset::show (std::ostream &o) const
{
  o << cov::format_ranges {cov};
}

std::unique_ptr <value>
value_aset::clone () const
{
  return std::make_unique <value_aset> (*this);
}

cmp_result
value_aset::cmp (value const &that) const
{
  if (auto v = value::as <value_aset> (&that))
    {
      auto const &cov2 = v->cov;

      cmp_result ret = compare (cov.size (), cov2.size ());
      if (ret != cmp_result::equal)
	return ret;

      for (size_t i = 0; i < cov.size (); ++i)
	if ((ret = compare (cov.at (i).start,
			    cov2.at (i).start)) != cmp_result::equal)
	  return ret;
	else if ((ret = compare (cov.at (i).length,
				 cov2.at (i).length)) != cmp_result::equal)
	  return ret;

      return cmp_result::equal;
    }
  else
    return cmp_result::fail;
}
