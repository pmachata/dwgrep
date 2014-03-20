#ifndef _VALFILE_H_
#define _VALFILE_H_

#include <memory>
#include <string>
#include <vector>

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
    ATTRIBUTE,
    LINE,
    LOCLIST_ENTRY,
    LOCLIST_OP,
  };

// We use this type to hold slots of value file.  Since we don't store
// variant discriminator with the slot itself, it needs to be
// destructed explicitly from the outside.  This is arranged between
// valfile and valfile_slot.  See valfile_slot::destroy.
union valfile_slot;

// valueref subclasses implement a view of a certain valfile slot.
// They are fairly cheap--take one word for vtable and typically
// another word for reference to underlying value.  valfile::get_slot
// returns this to represent all possible types through a unified
// interface.  valfile also has a number of methods for accessing
// slots by their proper type (see e.g. valfile::get_slot_str), in
// which case valueref is avoided altogether.
class valueref
{
public:
  virtual ~valueref () {}
  virtual void show (std::ostream &o) const = 0;
  virtual slot_type set_slot (valfile_slot &slt) const = 0;
};

// Values are like value references, but they own the memory for the
// held underlying value.  They are thus suitable for storing in
// vectors and such.
class value
  : public valueref
{
public:
  virtual std::unique_ptr <value> clone () const = 0;
};

typedef std::vector <std::unique_ptr <value> > value_vector_t;

// Builder of value subclasses.  VR is the corrsponding valueref
// subclass, T is the actual underlying type.
template <class VR, class T>
class value_holder
  : public value
{
  T m_value;
  VR m_ref;

public:
  value_holder (T const &v)
    : m_value (v)
    , m_ref (m_value)
  {}

  value_holder (value_holder const &that)
    : m_value (that.m_value)
    , m_ref (m_value)
  {}

  value_holder (value_holder &&that)
    : m_value (std::move (that))
    , m_ref (m_value)
  {}

  ~value_holder () = default;

  void
  show (std::ostream &o) const override
  {
    return m_ref.show (o);
  }

  slot_type
  set_slot (valfile_slot &slt) const override
  {
    return m_ref.set_slot (slt);
  }

  std::unique_ptr <value>
  clone () const override
  {
    return std::unique_ptr <value> { new value_holder <VR, T> (*this) };
  }
};

class valueref_cst
  : public valueref
{
  constant const &m_cst;

public:
  explicit valueref_cst (constant const &cst)
    : m_cst (cst)
  {}

  void show (std::ostream &o) const override;
  slot_type set_slot (valfile_slot &slt) const override;
};

typedef value_holder <valueref_cst, constant> value_cst;

class valueref_str
  : public valueref
{
  std::string const &m_str;

public:
  explicit valueref_str (std::string const &str)
    : m_str (str)
  {}

  void show (std::ostream &o) const override;
  slot_type set_slot (valfile_slot &slt) const override;
};

typedef value_holder <valueref_str, std::string> value_str;

class valueref_seq
  : public valueref
{
  value_vector_t const &m_seq;

public:
  explicit valueref_seq (value_vector_t const &seq)
    : m_seq (seq)
  {}

  void show (std::ostream &o) const override;
  slot_type set_slot (valfile_slot &slt) const override;
};

typedef value_holder <valueref_seq, value_vector_t> value_seq;

union valfile_slot
{
  constant cst;
  double flt;
  std::string str;
  value_vector_t seq;
  Dwarf_Die die;
  Dwarf_Attribute at;
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
  static std::unique_ptr <valfile> create (size_t n);
  static std::unique_ptr <valfile> copy (valfile const &that, size_t n);

  ~valfile ();

  std::unique_ptr <valueref> get_slot (size_t i) const;
  void invalidate_slot (size_t i);
  void set_slot (size_t i, valueref const &sv);

  slot_type get_slot_type (size_t i) const;

  // Specialized per-type slot accessors.
  std::string const &get_slot_str (size_t i) const;
  constant const &get_slot_cst (size_t i) const;
  value_vector_t const &get_slot_seq (size_t i) const;

  class const_iterator
    : public std::iterator <std::input_iterator_tag,
			    std::unique_ptr <valueref> >
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
