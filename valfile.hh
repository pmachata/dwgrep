#ifndef _VALFILE_H_
#define _VALFILE_H_

#include <memory>
#include <string>
#include <vector>
#include "make_unique.hh"

#include <elfutils/libdw.h>

#include "constant.hh"

// Value file is a specialized container type that's used for
// maintaining stacks of dwgrep values.
//
// The file is static in size.  Unlike std::array, the size can be
// determined at runtime, but like std::array, the size never changes
// during the lifetime of a value file.
//
// Types of values stored are determined at runtime as well, though
// there's a pre-coded set of values that a value file can hold.
//
// Value file is not a value type.  The whole file is allocated in a
// single blob on the heap.  Types of slots are stored in a byte array
// (see enum slot_type) separately from the slots, which allows for
// denser storage.  Slots themselves are allocated statically to fit
// the largest type that a value file can store.  Size of a value file
// is not stored explicitly, but is implicit as a dedicated END slot
// type.
//
// Due to the described storage format, a value file needs to closely
// cooperate with its slots on their construction and destruction.
// The class is fairly well encapsulated though, and apart from unusul
// interface, shouldn't require much care.
//
// Value file assume that the user knows which slots are taken and
// which are free.  This is so because in dwgrep language, source and
// destination slots of an operator can be always determined
// statically.  It is therefore an error to ask for a slot type (or a
// value) if it's either END or INVALID.  If it is desirable to store
// some sort of null value, that should be added as an additional slot
// type.  The user is not required to always know what the stored type
// is.  It is understood that that may be a dynamic property.
struct valfile;

namespace std
{
  // Because valfile is allocated specially, it needs to be destroyed
  // specially as well.  Using a custom deleter of std::unique_ptr
  // adds one pointer of overhead, which we don't care for.
  // Specialize instead the default deleter.  That is OK, because we
  // only ever store valfile pointer inside one type of unique_ptr.
  template <>
  struct default_delete <valfile>
  {
    void operator() (valfile *ptr) const;
  };
}

enum class slot_type: int8_t
  {
    END = -1,		// Denotes end of file.
    INVALID = 0,	// Empty slot.

    // Generic types:
    CST,
    FLT,
    STR,
    SEQ,

    // Backend types:
    DIE,
    ATTR,
    LINE,
    LOCLIST_ENTRY,
    LOCLIST_OP,
  };

// We use this type to hold slots of value file.  Since we don't store
// variant discriminator with the slot itself, it needs to be
// destructed explicitly from the outside.  This is arranged between
// valfile and valfile_slot.  See valfile_slot::destroy.
union valfile_slot;

// value references implement access to a value.  That can be either
// stored in a valfile slot, or somewhere else.
class valueref;

// values are like value references, but they carry around memory that
// they reference.  They can be used to initialize valfile slots.
class value;

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

struct attribute_slot
{
  Dwarf_Attribute attr;
  Dwarf_Off die_off;
};

class valueref
{
public:
  virtual ~valueref () {}
  virtual void show (std::ostream &o) const = 0;
  virtual std::unique_ptr <value> clone () const = 0;
};

class value
  : public valueref
{
public:
  virtual void move_to_slot (slot_idx i, valfile &vf) = 0;
};

// Specializations of this template implement the same interfaces as
// class value, but as static functions, and with an argument
// corresponding to the underlying value type added to the front.
// value_contr (below) calls into this.
template <class T>
class value_behavior;

typedef std::vector <std::unique_ptr <value> > value_vector_t;

template <class T>
class value_b
  : public value
{
  T m_value;

public:
  explicit value_b (T &&value)
    : m_value (std::move (value))
  {}

  void
  show (std::ostream &o) const override
  {
    value_behavior <T>::show (m_value, o);
  }

  std::unique_ptr <value>
  clone () const override
  {
    return value_behavior <T>::clone (m_value);
  }

  void
  move_to_slot (slot_idx i, valfile &vf) override
  {
    value_behavior <T>::move_to_slot (std::move (m_value), i, vf);
  }
};

template <class T>
class valueref_b
  : public valueref
{
  T const &m_value;

public:
  explicit valueref_b (T const &value)
    : m_value (value)
  {}

  void
  show (std::ostream &o) const override
  {
    value_behavior <T>::show (m_value, o);
  }

  std::unique_ptr <value>
  clone () const override
  {
    return value_behavior <T>::clone (m_value);
  }
};

template <>
struct value_behavior <constant>
{
  static void show (constant const &cst, std::ostream &o);
  static void move_to_slot (constant &&cst, slot_idx i, valfile &vf);
  static std::unique_ptr <value> clone (constant const &cst);
};

typedef value_b <constant> value_cst;
typedef valueref_b <constant> valueref_cst;

template <>
struct value_behavior <std::string>
{
  static void show (std::string const &str, std::ostream &o);
  static void move_to_slot (std::string &&str, slot_idx i, valfile &vf);
  static std::unique_ptr <value> clone (std::string const &str);
};

typedef value_b <std::string> value_str;
typedef valueref_b <std::string> valueref_str;

template <>
struct value_behavior <value_vector_t>
{
  static void show (value_vector_t const &vv, std::ostream &o);
  static void move_to_slot (value_vector_t &&vv, slot_idx i, valfile &vf);
  static std::unique_ptr <value> clone (value_vector_t const &vv);
};

typedef value_b <value_vector_t> value_seq;
typedef valueref_b <value_vector_t> valueref_seq;

template <>
struct value_behavior <Dwarf_Die>
{
  static void show (Dwarf_Die const &die, std::ostream &o);
  static void move_to_slot (Dwarf_Die &&die, slot_idx i, valfile &vf);
  static std::unique_ptr <value> clone (Dwarf_Die const &die);
};

typedef value_b <Dwarf_Die> value_die;
typedef valueref_b <Dwarf_Die> valueref_die;

template <>
struct value_behavior <attribute_slot>
{
  static void show (attribute_slot const &die, std::ostream &o);
  static void move_to_slot (attribute_slot &&die, slot_idx i, valfile &vf);
  static std::unique_ptr <value> clone (attribute_slot const &die);
};

typedef value_b <attribute_slot> value_attr;
typedef valueref_b <attribute_slot> valueref_attr;


// |-----------------+-------+--------------------------------|
// | type            | bytes |                                |
// |-----------------+-------+--------------------------------|
// | Dwarf_Die       | 32/16 |                                |
// | Dwarf_Attribute | 32/24 | 24/16 + 8/8 for DIE offset     |
// | Dwarf_Line *    | 8/4   | line_table_entry pseudo-DIE    |
// | ???             |       | location_list_entry pseudo-DIE |
// | Dwarf_Op        | 32/32 | location_list_op pseudo-DIE    |
// |-----------------+-------+--------------------------------|
// | constant        | 16/12 |                                |
// | flt             | 8/8   |                                |
// | std::string     | 8/4   |                                |
// | std::vector     | 24/12 |                                |
// |-----------------+-------+--------------------------------|
union valfile_slot
{
  constant cst;
  double flt;
  std::string str;
  value_vector_t seq;
  Dwarf_Die die;
  attribute_slot attr;
  Dwarf_Line *line;

  // Do-nothing ctor and dtor to make it possible to instantiate this
  // thing even though it contains types with non-trivial ctors.
  valfile_slot () {}
  ~valfile_slot () {}

private:
  friend class valfile;
  void destroy (slot_type t);
};

class valfile
{
  slot_type *m_types;
  valfile_slot m_slots[];

  struct allocd_block
  {
    unsigned char *buf;
    slot_type *types;
  };

  static allocd_block alloc (size_t n);

  explicit valfile (slot_type *types)
    : m_types (types)
  {}

public:
  typedef std::unique_ptr <valfile> uptr;
  static uptr create (size_t n);
  static uptr copy (valfile const &that, size_t size);

  ~valfile ();

  // These two calls are for debugging purposes.  If you call them,
  // they print out a warning message.  You are supposed to not care
  // about these constants at all, as each operator should know which
  // slots it operates with, and which slot to put the result in.
  size_t size () const;
  size_t capacity () const;

  void invalidate_slot (slot_idx i);

  // Polymorphic access to a slot.
  void set_slot (slot_idx i, value &&sv);
  void set_slot (slot_idx i, std::unique_ptr <value> sv);
  std::unique_ptr <valueref> get_slot (slot_idx i) const;

  // Typed access to a slot.
  slot_type get_slot_type (slot_idx i) const;

  void set_slot (slot_idx i, constant &&cst);
  constant const &get_slot_cst (slot_idx i) const;

  void set_slot (slot_idx i, std::string &&str);
  std::string const &get_slot_str (slot_idx i) const;

  void set_slot (slot_idx i, value_vector_t &&seq);
  value_vector_t const &get_slot_seq (slot_idx i) const;

  void set_slot (slot_idx i, Dwarf_Die const &die);
  Dwarf_Die const &get_slot_die (slot_idx i) const;

  void set_slot (slot_idx i, attribute_slot &&attr);
  attribute_slot const &get_slot_attr (slot_idx i) const;

  class const_iterator
    : public std::iterator <std::input_iterator_tag,
			    std::unique_ptr <value> >
  {
    friend class valfile;
    valfile const *m_seq;
    ssize_t m_i;

    void next ();

    explicit const_iterator (valfile const *s)
      : m_seq (s)
      , m_i (-1)
    {}

    const_iterator (valfile const *s, size_t i)
      : m_seq (s)
      , m_i (i)
    {
      next ();
    }

  public:
    const_iterator ()
      : m_seq (nullptr)
      , m_i (-1)
    {}

    const_iterator (const_iterator const &that)
      : m_seq (that.m_seq)
      , m_i (that.m_i)
    {}

    bool operator== (const_iterator that) const;
    bool operator!= (const_iterator that) const;

    const_iterator &operator++ ();
    const_iterator operator++ (int);

    std::unique_ptr <valueref> operator* () const;
    std::unique_ptr <valueref> operator-> () const;
  };

  const_iterator
  begin () const
  {
    return const_iterator (this, 0);
  }

  const_iterator
  end () const
  {
    return const_iterator (this);
  }
};

#endif /* _VALFILE_H_ */
