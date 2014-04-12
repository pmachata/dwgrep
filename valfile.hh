#ifndef _VALFILE_H_
#define _VALFILE_H_

#include <memory>
#include <vector>

#include "value.hh"

// Value file is a container type that's used for maintaining stacks
// of dwgrep values.  The file is static in size.  Unlike std::array,
// the size can be determined at runtime, but like std::array, the
// size never changes during the lifetime of a value file.
struct valfile;

// Strongly typed unsigned for representing slot indices.
class slot_idx
{
  unsigned m_value;

public:
  explicit slot_idx (unsigned value)
    : m_value (value)
  {}

  slot_idx (slot_idx const &that)
    : m_value (that.m_value)
  {}

  slot_idx
  operator= (slot_idx that)
  {
    m_value = that.m_value;
    return *this;
  }

  unsigned
  value () const
  {
    return m_value;
  }

  bool
  operator== (slot_idx that) const
  {
    return m_value == that.m_value;
  }

  bool
  operator!= (slot_idx that) const
  {
    return m_value != that.m_value;
  }

  slot_idx
  operator- (unsigned value) const
  {
    assert (value <= m_value);
    return slot_idx (m_value - value);
  }
};

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
    m_values[i.value ()] = std::move (vp);
  }

  std::unique_ptr <value>
  take_slot (slot_idx idx)
  {
    size_t i = idx.value ();
    return std::move (m_values[i]);
  }

  value const &
  get_slot (slot_idx idx) const
  {
    size_t i = idx.value ();
    assert (m_values[i] != nullptr);
    return *m_values[i].get ();
  }

  template <class T>
  T *get_slot_as (slot_idx i) const
  {
    value *vp = m_values[i.value ()].get ();
    assert (vp != nullptr);
    return dynamic_cast <T *> (vp);
  }

  bool operator< (valfile const &that) const;
  bool operator== (valfile const &that) const;
};

#endif /* _VALFILE_H_ */
