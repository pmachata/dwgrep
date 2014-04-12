#ifndef _VALUE_H_
#define _VALUE_H_

#include <memory>
#include <iosfwd>
#include <vector>
#include <string>
#include <elfutils/libdw.h>

#include "constant.hh"

enum class cmp_result
  {
    less,
    equal,
    greater,
    fail,
  };

class value
{
public:
  virtual ~value () {}
  virtual void show (std::ostream &o) const = 0;
  virtual std::unique_ptr <value> clone () const = 0;
  virtual constant get_type_const () const = 0;
  virtual cmp_result cmp (value const &that) const = 0;
};

class value_cst
  : public value
{
  constant m_cst;

public:
  explicit value_cst (constant cst)
    : m_cst {cst}
  {}

  constant const &get_constant () const
  { return m_cst; }

  void show (std::ostream &o) const override;
  std::unique_ptr <value> clone () const override;
  constant get_type_const () const override;
  cmp_result cmp (value const &that) const override;
};

class value_str
  : public value
{
  std::string m_str;

public:
  explicit value_str (std::string &&str)
    : m_str {std::move (str)}
  {}

  std::string const &get_string () const
  { return m_str; }

  void show (std::ostream &o) const override;
  std::unique_ptr <value> clone () const override;
  constant get_type_const () const override;
  cmp_result cmp (value const &that) const override;
};

class value_seq
  : public value
{
public:
  typedef std::vector <std::unique_ptr <value> > seq_t;

private:
  seq_t m_seq;

public:
  explicit value_seq (seq_t &&seq)
    : m_seq {std::move (seq)}
  {}

  seq_t const &get_seq () const
  { return m_seq; }

  seq_t &&move_seq ()
  { return std::move (m_seq); }

  void show (std::ostream &o) const override;
  std::unique_ptr <value> clone () const override;
  constant get_type_const () const override;
  cmp_result cmp (value const &that) const override;
};

class value_die
  : public value
{
  Dwarf_Die m_die;

public:
  explicit value_die (Dwarf_Die die)
    : m_die (die)
  {}

  Dwarf_Die const &get_die () const
  { return m_die; }

  void show (std::ostream &o) const override;
  std::unique_ptr <value> clone () const override;
  constant get_type_const () const override;
  cmp_result cmp (value const &that) const override;
};

class value_attr
  : public value
{
  Dwarf_Attribute m_attr;
  Dwarf_Die m_die;

public:
  value_attr (Dwarf_Attribute attr, Dwarf_Die die)
    : m_attr (attr)
    , m_die (die)
  {}

  Dwarf_Attribute const &get_attr () const
  { return m_attr; }

  Dwarf_Die get_die () const
  { return m_die; }

  void show (std::ostream &o) const override;
  std::unique_ptr <value> clone () const override;
  constant get_type_const () const override;
  cmp_result cmp (value const &that) const override;
};


#endif /* _VALUE_H_ */
