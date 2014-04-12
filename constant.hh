#ifndef _CST_H_
#define _CST_H_

#include <cassert>
#include <iosfwd>
#include <cstdint>

class constant;

class constant_dom
{
public:
  virtual ~constant_dom () {}
  virtual void show (uint64_t c, std::ostream &o) const = 0;

  // Whether values of this domain are signed.
  virtual bool sign () const = 0;

  // Whether this domain is considered safe for integer arithmetic.
  // (E.g. named constants wouldn't.)
  virtual bool safe_arith () const { return false; }

  // When doing arithmetic, whether this domain is considered plain
  // and should be given up for the other one.  (E.g. hex domain
  // wouldn't.)
  virtual bool plain () const { return false; }
};

// Two trivial domains for unnamed constants: one for signed, one for
// unsigned values.
extern constant_dom const &signed_constant_dom;
extern constant_dom const &unsigned_constant_dom;

// A domain for hex- and oct-formated unsigned values.
extern constant_dom const &hex_constant_dom;
extern constant_dom const &oct_constant_dom;

// A domain for boolean-formated unsigned values.
extern constant_dom const &bool_constant_dom;

extern constant_dom const &column_number_dom;
extern constant_dom const &line_number_dom;

class constant
{
  uint64_t m_value;
  constant_dom const *m_dom;

public:
  constant ()
    : m_value {0}
    , m_dom {nullptr}
  {}

  constant (uint64_t value, constant_dom const *dom)
    : m_value {value}
    , m_dom {dom}
  {}

  constant (constant const &copy) = default;

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
