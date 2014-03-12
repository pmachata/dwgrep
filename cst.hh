#ifndef _CST_H_
#define _CST_H_

#include <cassert>
#include <iosfwd>

class cst;

class cst_dom
{
public:
  virtual void format (cst c, std::ostream &o) const = 0;
  virtual ~cst_dom () {}
};

struct untyped_cst_dom_t
  : public cst_dom
{
  virtual void format (cst c, std::ostream &o) const;
};

extern untyped_cst_dom_t const untyped_cst_dom;

class cst
{
  uint64_t m_value;
  cst_dom const *m_dom;

public:
  cst (uint64_t value, cst_dom const *dom)
    : m_value (value)
    , m_dom (dom)
  {}

  cst_dom const *dom () const
  {
    return m_dom;
  }

  uint64_t value () const
  {
    return m_value;
  }

  // Parse constant.
  static cst parse (std::string str);

  // Parse attribute reference.
  static cst parse_attr (std::string str);

  // Parse tag reference.
  static cst parse_tag (std::string str);
};

#endif /* _CST_H_ */
