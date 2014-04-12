#ifndef _VALFILE_H_
#define _VALFILE_H_

#include <memory>
#include <string>
#include <vector>
#include "make_unique.hh"

#include <elfutils/libdw.h>

#include "constant.hh"

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
  Dwarf_Off m_paroff;

public:
  value_attr (Dwarf_Attribute attr, Dwarf_Off paroff)
    : m_attr (attr)
    , m_paroff (paroff)
  {}

  Dwarf_Attribute const &get_attr () const
  { return m_attr; }

  Dwarf_Off get_parent_off () const
  { return m_paroff; }

  void show (std::ostream &o) const override;
  std::unique_ptr <value> clone () const override;
  constant get_type_const () const override;
  cmp_result cmp (value const &that) const override;
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
    assert (m_values[i] != nullptr);
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
  T const *get_slot_as (slot_idx i) const
  {
    value const *vp = m_values[i.value ()].get ();
    assert (vp != nullptr);
    return dynamic_cast <T const *> (vp);
  }

  bool operator< (valfile const &that) const;
  bool operator== (valfile const &that) const;
};

#endif /* _VALFILE_H_ */
