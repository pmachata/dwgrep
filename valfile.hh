#ifndef _VALFILE_H_
#define _VALFILE_H_

#include <memory>
#include <vector>

#include "value.hh"
#include "slot_idx.hh"

// Value file is a container type that's used for maintaining stacks
// of dwgrep values.  The file is static in size.  Unlike std::array,
// the size can be determined at runtime, but like std::array, the
// size never changes during the lifetime of a value file.
class valfile
{
  std::vector <std::unique_ptr <value> > m_values;

public:
  typedef std::unique_ptr <valfile> uptr;
  explicit valfile (size_t n)
    : m_values {n}
  {}

  valfile (valfile const &other);
  ~valfile () = default;

  size_t
  size () const
  {
    return m_values.size ();
  }

  void
  set_slot (slot_idx i, std::unique_ptr <value> vp)
  {
    m_values[i] = std::move (vp);
  }

  std::unique_ptr <value>
  release_slot (slot_idx idx)
  {
    return std::move (m_values[idx]);
  }

  value const &
  get_slot (slot_idx idx) const
  {
    assert (m_values[idx] != nullptr);
    return *m_values[idx].get ();
  }

  template <class T>
  T *get_slot_as (slot_idx idx) const
  {
    value *vp = m_values[idx].get ();
    assert (vp != nullptr);
    return value::as <T> (vp);
  }

  bool operator< (valfile const &that) const;
  bool operator== (valfile const &that) const;
};

#endif /* _VALFILE_H_ */
