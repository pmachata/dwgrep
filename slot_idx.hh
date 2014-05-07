#ifndef _SLOT_IDX_H_
#define _SLOT_IDX_H_

#include <iosfwd>
#include <cassert>

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

  bool
  operator< (slot_idx that) const
  {
    return m_value < that.m_value;
  }

  bool
  operator== (unsigned val) const
  {
    return m_value != val;
  }

  slot_idx
  operator- (unsigned value) const
  {
    assert (value <= m_value);
    return slot_idx (m_value - value);
  }
};

std::ostream &operator<< (std::ostream &o, slot_idx const &idx);

#endif /* _SLOT_IDX_H_ */
