#ifndef _CST_H_
#define _CST_H_

#include <cassert>
#include <iosfwd>
#include <cstdint>
#include <cstddef> // Workaround for gmpxx.h.
#include <gmpxx.h>

class constant;

class constant_dom
{
public:
  virtual ~constant_dom () {}
  virtual void show (mpz_class const &c, std::ostream &o) const = 0;
  virtual std::string name () const = 0;

  // Whether this domain is considered safe for integer arithmetic.
  // (E.g. named constants aren't.)
  virtual bool safe_arith () const { return false; }

  // When doing arithmetic, whether this domain is considered plain
  // and should be given up for the other one.  (E.g. hex domain
  // wouldn't.)
  virtual bool plain () const { return false; }
};

struct numeric_constant_dom_t
  : public constant_dom
{
  std::string m_name;

public:
  explicit numeric_constant_dom_t (std::string const &name)
    : m_name {name}
  {}

  void show (mpz_class const &v, std::ostream &o) const override;
  bool safe_arith () const override { return true; }
  bool plain () const override { return true; }

  std::string
  name () const override
  {
    return m_name;
  }
};

// Domains for unnamed constants.
extern constant_dom const &dec_constant_dom;
extern constant_dom const &hex_constant_dom;
extern constant_dom const &oct_constant_dom;
extern constant_dom const &bin_constant_dom;

// A domain for boolean-formated unsigned values.
extern constant_dom const &bool_constant_dom;

extern constant_dom const &column_number_dom;
extern constant_dom const &line_number_dom;

class constant
{
  mpz_class m_value;
  constant_dom const *m_dom;

public:
  constant ()
    : m_value {0}
    , m_dom {nullptr}
  {}

  template <class T>
  constant (T const &value, constant_dom const *dom)
    : m_value (value)
    , m_dom {dom}
  {}

  constant (constant const &copy) = default;

  constant_dom const *dom () const
  {
    return m_dom;
  }

  mpz_class const &value () const
  {
    return m_value;
  }

  bool operator< (constant that) const;
  bool operator> (constant that) const;
  bool operator== (constant that) const;
  bool operator!= (constant that) const;
};

std::ostream &operator<< (std::ostream &o, constant cst);

void check_arith (constant const &cst_a, constant const &cst_b);

// For two different domains, complain about comparisons that
// don't have at least one comparand signed_constant_dom or
// unsigned_constant_dom.
void check_constants_comparable (constant const &cst_a, constant const &cst_b);

#endif /* _CST_H_ */
