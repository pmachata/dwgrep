#ifndef _VALFILE_H_
#define _VALFILE_H_

#include <memory>
#include <vector>

#include "value.hh"

// Value file is a container type that's used for maintaining stacks
// of dwgrep values.
class valfile
{
  std::vector <std::unique_ptr <value> > m_values;

public:
  typedef std::unique_ptr <valfile> uptr;

  valfile () = default;
  valfile (valfile const &other);
  valfile (valfile &&other) = default;
  ~valfile () = default;

  size_t
  size () const
  {
    return m_values.size ();
  }

  void
  push (std::unique_ptr <value> vp)
  {
    m_values.push_back (std::move (vp));
  }

  std::unique_ptr <value>
  pop ()
  {
    assert (! m_values.empty ());
    auto ret = std::move (m_values.back ());
    m_values.pop_back ();
    return ret;
  }

  value &
  top ()
  {
    assert (! m_values.empty ());
    return *m_values.back ().get ();
  }

  value &
  below ()
  {
    assert (m_values.size () > 1);
    return *(m_values.rbegin () + 1)->get ();
  }

  template <class T>
  T *
  top_as ()
  {
    value const &ret = top ();
    return value::as <T> (const_cast <value *> (&ret));
  }

  template <class T>
  T *
  below_as ()
  {
    value const &ret = below ();
    return value::as <T> (const_cast <value *> (&ret));
  }

  bool operator< (valfile const &that) const;
  bool operator== (valfile const &that) const;
};

#endif /* _VALFILE_H_ */
