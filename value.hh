#ifndef _VALUE_H_
#define _VALUE_H_

#include <memory>
#include <iosfwd>
#include <vector>
#include <string>
#include <elfutils/libdw.h>

#include "constant.hh"
#include "dwgrep.hh"

enum class cmp_result
  {
    less,
    equal,
    greater,
    fail,
  };

// We use this to keep track of types of instances of subclasses of
// class value.  value::as uses this to avoid having to dynamic_cast,
// which is needlessly flexible and slow for our purposes.
class value_type
{
  uint8_t m_code;

public:
  static value_type const none;

  explicit value_type (uint8_t code)
    : m_code {code}
  {}

  value_type (value_type const &that)
    : m_code {that.m_code}
  {}

  bool
  operator< (value_type that) const
  {
    return m_code < that.m_code;
  }

  bool
  operator== (value_type that) const
  {
    return m_code == that.m_code;
  }

  bool
  operator!= (value_type that) const
  {
    return m_code != that.m_code;
  }
};

class value
{
  value_type const m_type;
  size_t m_pos;

protected:
  value (value_type t, size_t pos)
    : m_type {t}
    , m_pos {pos}
  {}

  value (value const &that) = default;

public:
  value_type get_type () { return m_type; }

  virtual ~value () {}
  virtual void show (std::ostream &o) const = 0;
  virtual std::unique_ptr <value> clone () const = 0;
  virtual constant get_type_const () const = 0;
  virtual cmp_result cmp (value const &that) const = 0;

  void
  set_pos (size_t pos)
  {
    m_pos = pos;
  }

  size_t
  get_pos () const
  {
    return m_pos;
  }

  template <class T>
  bool
  is () const
  {
    return T::vtype == m_type;
  }

  template <class T>
  static T *
  as (value *val)
  {
    assert (val != nullptr);
    if (! val->is <T> ())
      return nullptr;
    return static_cast <T *> (val);
  }

  template <class T>
  static T const *
  as (value const *val)
  {
    assert (val != nullptr);
    if (! val->is <T> ())
      return nullptr;
    return static_cast <T const *> (val);
  }
};

std::ostream &operator<< (std::ostream &o, value const &v);

class value_cst
  : public value
{
  constant m_cst;

public:
  static value_type const vtype;

  value_cst (constant cst, size_t pos)
    : value {vtype, pos}
    , m_cst {cst}
  {}

  value_cst (value_cst const &that) = default;

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
  static value_type const vtype;

  value_str (std::string &&str, size_t pos)
    : value {vtype, pos}
    , m_str {std::move (str)}
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
  std::shared_ptr <seq_t> m_seq;

public:
  static value_type const vtype;

  value_seq (seq_t &&seq, size_t pos)
    : value {vtype, pos}
    , m_seq {std::make_shared <seq_t> (std::move (seq))}
  {}

  value_seq (std::shared_ptr <seq_t> seqp, size_t pos)
    : value {vtype, pos}
    , m_seq {seqp}
  {}

  value_seq (value_seq const &that);

  std::shared_ptr <seq_t>
  get_seq () const
  {
    return m_seq;
  }

  void show (std::ostream &o) const override;
  std::unique_ptr <value> clone () const override;
  constant get_type_const () const override;
  cmp_result cmp (value const &that) const override;
};

class tree;
class frame;
class scope;

class value_closure
  : public value
{
  std::unique_ptr <tree> m_t;
  dwgrep_graph::sptr m_q;
  std::shared_ptr <scope> m_scope;
  std::shared_ptr <frame> m_frame;

public:
  static value_type const vtype;

  value_closure (tree const &t, dwgrep_graph::sptr q,
		 std::shared_ptr <scope> scope, std::shared_ptr <frame> frame,
		 size_t pos);
  value_closure (value_closure const &that);
  ~value_closure();

  tree const &get_tree () const
  { return *m_t; }

  dwgrep_graph::sptr get_graph () const
  { return m_q; }

  std::shared_ptr <scope> get_scope () const
  { return m_scope; }

  std::shared_ptr <frame> get_frame () const
  { return m_frame; }

  void show (std::ostream &o) const override;
  std::unique_ptr <value> clone () const override;
  constant get_type_const () const override;
  cmp_result cmp (value const &that) const override;
};

class value_die
  : public value
{
  Dwarf_Die m_die;
  dwgrep_graph::sptr m_gr;

public:
  static value_type const vtype;

  value_die (dwgrep_graph::sptr gr, Dwarf_Die die, size_t pos)
    : value {vtype, pos}
    , m_die (die)
    , m_gr {gr}
  {}

  value_die (value_die const &that) = default;

  Dwarf_Die &get_die ()
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
  dwgrep_graph::sptr m_gr;

public:
  static value_type const vtype;

  value_attr (dwgrep_graph::sptr gr,
	      Dwarf_Attribute attr, Dwarf_Die die, size_t pos)
    : value {vtype, pos}
    , m_attr (attr)
    , m_die (die)
    , m_gr {gr}
  {}

  value_attr (value_attr const &that) = default;

  Dwarf_Attribute &get_attr ()
  { return m_attr; }

  Dwarf_Die &get_die ()
  { return m_die; }

  void show (std::ostream &o) const override;
  std::unique_ptr <value> clone () const override;
  constant get_type_const () const override;
  cmp_result cmp (value const &that) const override;
};


#endif /* _VALUE_H_ */
