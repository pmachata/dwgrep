#ifndef _VALFILE_H_
#define _VALFILE_H_

#include <memory>
#include <string>
#include <iostream>
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
// Slots themselves are allocated statically to fit the largest type
// that a value file can store.
//
// Value file is not a value type.  The whole file is allocated in a
// single blob on the heap.  Types of slots are stored in a byte array
// (see enum slot_type) separately from the slots themselves, which
// allows for denser storage.  Size of a value file is not stored
// explicitly, but is implicit as a dedicated END slot type.
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
  // Specialize instead the default deleter.
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
// another word for reference to underlying value.  Files allow access
// to each slot by its proper type (see e.g. valfile::get_slot_str),
// in which case the reference is returned as value type.  They also
// allow a general access whereby an instance is allocated on the heap
// and returned wrapped in a unique_ptr.
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

  valfile (slot_type *types, size_t n)
    : m_types (types)
  {}

  struct allocd_block
  {
    unsigned char *buf;
    slot_type *types;
  };

  static allocd_block
  alloc (size_t n)
  {
    size_t sz1 = offsetof (valfile, m_slots[n]);
    size_t sz = sz1 + sizeof (slot_type) * (n + 1);
    unsigned char *buf = new unsigned char[sz];
    return { buf, reinterpret_cast <slot_type *> (buf + sz1) };
  }

public:
  static std::unique_ptr <valfile>
  create (size_t n)
  {
    allocd_block b = alloc (n);

    for (size_t i = 0; i < n; ++i)
      b.types[i] = slot_type::INVALID;
    b.types[n] = slot_type::END;

    return std::unique_ptr <valfile> { new (b.buf) valfile (b.types, n) };
  }

  static std::unique_ptr <valfile>
  copy (valfile const &that, size_t n)
  {
    assert (that.m_types[n] == slot_type::END);
    allocd_block b = alloc (n);

    for (size_t i = 0; i < n + 1; ++i)
      b.types[i] = that.m_types[i];

    return std::unique_ptr <valfile> { new (b.buf) valfile (b.types, n) };
  }

  ~valfile ()
  {
    for (size_t i = 0; m_types[i] != slot_type::END; ++i)
      m_slots[i].destroy (m_types[i]);
    // XXX destroy values in m_slots according to corresponding value
    // in m_types
  }

  slot_type
  get_slot_type (size_t i) const
  {
    assert (m_types[i] != slot_type::END);
    assert (m_types[i] != slot_type::INVALID);
    return m_types[i];
  }

  std::string const &
  get_slot_str (size_t i) const
  {
    assert (m_types[i] == slot_type::STR);
    return m_slots[i].str;
  }

  constant const &
  get_slot_cst (size_t i) const
  {
    assert (m_types[i] == slot_type::CST);
    return m_slots[i].cst;
  }

  value_vector_t const &
  get_slot_seq (size_t i) const
  {
    assert (m_types[i] == slot_type::SEQ);
    return m_slots[i].seq;
  }

  // General accessor for all slot types.
  std::unique_ptr <valueref>
  get_slot (size_t i) const
  {
    switch (m_types[i])
      {
      case slot_type::FLT:
      case slot_type::DIE:
      case slot_type::ATTRIBUTE:
      case slot_type::LINE:
	assert (!"not yet implemented");
	abort ();

      case slot_type::CST:
	return std::unique_ptr <valueref>
	  { new valueref_cst { m_slots[i].cst } };

      case slot_type::STR:
	return std::unique_ptr <valueref>
	  { new valueref_str { m_slots[i].str } };

      case slot_type::SEQ:
	return std::unique_ptr <valueref>
	  { new valueref_seq { m_slots[i].seq } };

      case slot_type::LOCLIST_ENTRY:
      case slot_type::LOCLIST_OP:
	assert (!"not implemented");
	abort ();

      case slot_type::INVALID:
	assert (m_types[i] != slot_type::INVALID);
	abort ();

      case slot_type::END:
	assert (m_types[i] != slot_type::END);
	abort ();
      }

    assert (m_types[i] != m_types[i]);
    abort ();
  }

  void
  invalidate_slot (size_t i)
  {
    assert (m_types[i] != slot_type::END);
    assert (m_types[i] != slot_type::INVALID);
    m_slots[i].destroy (m_types[i]);
    m_types[i] = slot_type::INVALID;
  }

  void
  set_slot (size_t i, valueref const &sv)
  {
    assert (m_types[i] == slot_type::INVALID);
    m_types[i] = sv.set_slot (m_slots[i]);
  }

  class const_iterator
    : public std::iterator <std::input_iterator_tag,
			    std::unique_ptr <valueref> >
  {
    friend class valfile;
    valfile const *m_seq;
    ssize_t m_i;

    void
    check ()
    {
      assert (m_i != -1);
      while (m_seq->m_types[m_i] == slot_type::INVALID)
	++m_i;

      if (m_seq->m_types[m_i] == slot_type::END)
	m_i = -1;
    }

    explicit const_iterator (valfile const *s)
      : m_seq (s)
      , m_i (-1)
    {}

    const_iterator (valfile const *s, size_t i)
      : m_seq (s)
      , m_i (i)
    {
      check ();
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

    bool
    operator== (const_iterator that) const
    {
      return m_seq == that.m_seq
	&& m_i == that.m_i;
    }

    bool
    operator!= (const_iterator that) const
    {
      return !(*this == that);
    }

    const_iterator &
    operator++ ()
    {
      assert (m_i != -1);
      ++m_i;
      check ();
      return *this;
    }

    const_iterator
    operator++ (int)
    {
      const_iterator tmp = *this;
      ++*this;
      return tmp;
    }

    std::unique_ptr <valueref>
    operator* () const
    {
      assert (m_i != -1);
      return m_seq->get_slot (m_i);
    }

    std::unique_ptr <valueref>
    operator-> () const
    {
      return **this;
    }
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

void
std::default_delete <valfile>::operator() (valfile *ptr) const
{
  ptr->~valfile ();
  delete[] (unsigned char *) ptr;
}

#endif /* _VALFILE_H_ */
