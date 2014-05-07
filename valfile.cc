#include "valfile.hh"

valfile::valfile (valfile const &that)
{
  for (auto const &vf: that.m_values)
    if (vf != nullptr)
      m_values.push_back (vf->clone ());
    else
      m_values.push_back (nullptr);
}

namespace
{
  int
  compare_vf (std::vector <std::unique_ptr <value> > const &a,
	      std::vector <std::unique_ptr <value> > const &b)
  {
    // There shouldn't really be stacks of different sizes in one
    // program.
    assert (a.size () == b.size ());

    // The stack that has nullptr where the other has non-nullptr is
    // smaller.
    {
      auto it = a.begin ();
      auto jt = b.begin ();
      for (; it != a.end (); ++it, ++jt)
	if (*it == nullptr && *jt != nullptr)
	  return -1;
	else if (*it != nullptr && *jt == nullptr)
	  return 1;
    }

    // The stack with "smaller" types is smaller.
    {
      auto it = a.begin ();
      auto jt = b.begin ();
      for (; it != a.end (); ++it, ++jt)
	if (*it != nullptr && *jt != nullptr)
	  {
	    if ((*it)->get_type () < (*jt)->get_type ())
	      return -1;
	    else if ((*jt)->get_type () < (*it)->get_type ())
	      return 1;
	  }
    }

    // We have the same number of slots with values of the same type.
    // Now compare the values directly.
    {
      auto it = a.begin ();
      auto jt = b.begin ();
      for (; it != a.end (); ++it, ++jt)
	if (*it != nullptr && *jt != nullptr)
	  switch ((*it)->cmp (**jt))
	    {
	    case cmp_result::fail:
	      assert (! "Comparison of same-typed slots shouldn't fail!");
	      abort ();
	    case cmp_result::less:
	      return -1;
	    case cmp_result::greater:
	      return 1;
	    case cmp_result::equal:
	      break;
	    }
    }

    // The stacks are the same!
    return 0;
  }
}

bool
valfile::operator< (valfile const &that) const
{
  return compare_vf (m_values, that.m_values) < 0;
}

bool
valfile::operator== (valfile const &that) const
{
  return compare_vf (m_values, that.m_values) == 0;
}
