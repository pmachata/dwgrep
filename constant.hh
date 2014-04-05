#ifndef _CST_H_
#define _CST_H_

#include <cassert>
#include <iosfwd>

class constant;

class constant_dom
{
public:
  virtual ~constant_dom () {}
  virtual void show (uint64_t c, std::ostream &o) const = 0;
  virtual bool sign () const = 0;
};

// Two trivial domains for unnamed constants: one for signed, one for
// unsigned values.
extern constant_dom const &signed_constant_dom;
extern constant_dom const &unsigned_constant_dom;

// A domain for hex-formated unsigned values (perhaps addresses).
extern constant_dom const &hex_constant_dom;

class constant
{
  uint64_t m_value;
  constant_dom const *m_dom;

public:
  constant (uint64_t value, constant_dom const *dom)
    : m_value (value)
    , m_dom (dom)
  {}

  constant_dom const *dom () const
  {
    return m_dom;
  }

  uint64_t value () const
  {
    return m_value;
  }

  // Parse constant.
  static constant parse (std::string str);

  // Parse attribute reference.
  static constant parse_attr (std::string str);

  // Parse tag reference.
  static constant parse_tag (std::string str);

  bool operator< (constant that) const;
  bool operator> (constant that) const;
  bool operator== (constant that) const;
  bool operator!= (constant that) const;
};

std::ostream &operator<< (std::ostream &o, constant cst);

#endif /* _CST_H_ */
