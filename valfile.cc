#include <iostream>

#include "valfile.hh"
#include "make_unique.hh"

void
std::default_delete <valfile>::operator() (valfile *ptr) const
{
  ptr->~valfile ();
  delete[] (unsigned char *) ptr;
}

void
value_behavior <constant>::show (constant const&cst, std::ostream &o)
{
  o << cst;
}

void
value_behavior <constant>::move_to_slot (constant &&cst,
					 slot_idx i, valfile &vf)
{
  vf.set_slot (i, std::move (cst));
}

std::unique_ptr <value>
value_behavior <constant>::clone (constant const&cst)
{
  return std::make_unique <value_cst> (constant (cst));
}

void
value_behavior <std::string>::show (std::string const &str, std::ostream &o)
{
  o << str;
}

void
value_behavior <std::string>::move_to_slot (std::string &&str,
					    slot_idx i, valfile &vf)
{
  vf.set_slot (i, std::move (str));
}

std::unique_ptr <value>
value_behavior <std::string>::clone (std::string const&cst)
{
  return std::make_unique <value_str> (std::string (cst));
}

void
value_behavior <value_vector_t>::show (value_vector_t const &vv,
				       std::ostream &o)
{
  o << "[";
  bool seen = false;
  for (auto const &v: vv)
    {
      if (seen)
	o << ", ";
      seen = true;
      v->show (o);
    }
  o << "]";
}

void
value_behavior <value_vector_t>::move_to_slot (value_vector_t &&vv,
					       slot_idx i, valfile &vf)
{
  vf.set_slot (i, std::move (vv));
}

std::unique_ptr <value>
value_behavior <value_vector_t>::clone (value_vector_t const&vv)
{
  value_vector_t vv2;
  for (auto const &v: vv)
    vv2.emplace_back (std::move (v->clone ()));
  return std::make_unique <value_seq> (std::move (vv2));
}

void
value_behavior <Dwarf_Die>::show (Dwarf_Die const &die, std::ostream &o)
{
  o << "[" << std::hex
    << dwarf_dieoffset ((Dwarf_Die *) &die) << "]";
}

void
value_behavior <Dwarf_Die>::move_to_slot (Dwarf_Die &&die,
					  slot_idx i, valfile &vf)
{
  vf.set_slot (i, die);
}

std::unique_ptr <value>
value_behavior <Dwarf_Die>::clone (Dwarf_Die const &die)
{
  return std::make_unique <value_die> (Dwarf_Die (die));
}

void
value_behavior <attribute_slot>::show (attribute_slot const &attr,
				       std::ostream &o)
{
  o << "(attribute, NYI)";
}

void
value_behavior <attribute_slot>::move_to_slot (attribute_slot &&attr,
					       slot_idx i, valfile &vf)
{
  vf.set_slot (i, std::move (attr));
}

std::unique_ptr <value>
value_behavior <attribute_slot>::clone (attribute_slot const &attr)
{
  return std::make_unique <value_attr> (attribute_slot (attr));
}

void
valfile_slot::destroy (slot_type t)
{
  switch (t)
    {
    case slot_type::INVALID:
    case slot_type::FLT:
    case slot_type::DIE:
    case slot_type::ATTR:
    case slot_type::LINE:
      return;

    case slot_type::CST:
      cst.~constant ();
      return;

    case slot_type::STR:
      str.~basic_string ();
      return;

    case slot_type::SEQ:
      seq.~vector ();
      return;

    case slot_type::LOCLIST_ENTRY:
    case slot_type::LOCLIST_OP:
      assert (!"not implemented");
      abort ();

    case slot_type::END:
      assert (t != slot_type::END);
      abort ();
    }

  assert (t != t);
  abort ();
}

valfile::allocd_block
valfile::alloc (size_t n)
{
  size_t sz1 = offsetof (valfile, m_slots[n]);
  size_t sz = sz1 + sizeof (slot_type) * (n + 1);
  unsigned char *buf = new unsigned char[sz];

  auto types = reinterpret_cast <slot_type *> (buf + sz1);
  for (size_t i = 0; i < n; ++i)
    types[i] = slot_type::INVALID;
  types[n] = slot_type::END;

  return { buf, types };
}

std::unique_ptr <valfile>
valfile::create (size_t n)
{
  allocd_block b = alloc (n);
  return std::unique_ptr <valfile> { new (b.buf) valfile (b.types) };
}

std::unique_ptr <valfile>
valfile::copy (valfile const &that, size_t size)
{
  assert (that.m_types[size] == slot_type::END);

  allocd_block b = alloc (size);
  auto copy = std::unique_ptr <valfile> (new (b.buf) valfile (b.types));

  for (size_t i = 0; i < size; ++i)
    if (that.m_types[i] != slot_type::INVALID)
      {
	std::unique_ptr <value> vp = that.get_slot (slot_idx (i))->clone ();
	copy->set_slot (slot_idx (i), std::move (vp));
      }
  return copy;
}

valfile::~valfile ()
{
  for (size_t i = 0; m_types[i] != slot_type::END; ++i)
    m_slots[i].destroy (m_types[i]);
}

namespace
{
  void dontuse ()
  {
    static bool seen = false;
    if (seen)
      return;
    seen = true;
    std::cout << "valfile::size and valfile::capacity are fairly expensive\n"
	      << "it should be possible to determine these constants statically"
	      << "\nfor any given dwgrep expression\n";
  }
}

size_t
valfile::size () const
{
  dontuse ();

  size_t n = 0;
  for (size_t i = 0; m_types[i] != slot_type::END; ++i)
    if (m_types[i] != slot_type::INVALID)
      ++n;

  return n;
}

size_t
valfile::capacity () const
{
  dontuse ();

  size_t i = 0;
  while (m_types[i] != slot_type::END)
    ++i;

  return i;
}

slot_type
valfile::get_slot_type (slot_idx i) const
{
  assert (m_types[i.value ()] != slot_type::END);
  assert (m_types[i.value ()] != slot_type::INVALID);
  return m_types[i.value ()];
}

std::string const &
valfile::get_slot_str (slot_idx i) const
{
  assert (m_types[i.value ()] == slot_type::STR);
  return m_slots[i.value ()].str;
}

constant const &
valfile::get_slot_cst (slot_idx i) const
{
  assert (m_types[i.value ()] == slot_type::CST);
  return m_slots[i.value ()].cst;
}

value_vector_t const &
valfile::get_slot_seq (slot_idx i) const
{
  assert (m_types[i.value ()] == slot_type::SEQ);
  return m_slots[i.value ()].seq;
}

Dwarf_Die const &
valfile::get_slot_die (slot_idx i) const
{
  assert (m_types[i.value ()] == slot_type::DIE);
  return m_slots[i.value ()].die;
}

attribute_slot const &
valfile::get_slot_attr (slot_idx i) const
{
  assert (m_types[i.value ()] == slot_type::ATTR);
  return m_slots[i.value ()].attr;
}

// General accessor for all slot types.
std::unique_ptr <valueref>
valfile::get_slot (slot_idx i) const
{
  switch (m_types[i.value ()])
    {
    case slot_type::FLT:
    case slot_type::LINE:
      assert (!"not yet implemented");
      abort ();

    case slot_type::DIE:
      return std::make_unique <valueref_die> (m_slots[i.value ()].die);

    case slot_type::ATTR:
      return std::make_unique <valueref_attr> (m_slots[i.value ()].attr);

    case slot_type::CST:
      return std::make_unique <valueref_cst> (m_slots[i.value ()].cst);

    case slot_type::STR:
      return std::make_unique <valueref_str> (m_slots[i.value ()].str);

    case slot_type::SEQ:
      return std::make_unique <valueref_seq> (m_slots[i.value ()].seq);

    case slot_type::LOCLIST_ENTRY:
    case slot_type::LOCLIST_OP:
      assert (!"not implemented");
      abort ();

    case slot_type::INVALID:
      assert (m_types[i.value ()] != slot_type::INVALID);
      abort ();

    case slot_type::END:
      assert (m_types[i.value ()] != slot_type::END);
      abort ();
    }

  assert (m_types[i.value ()] != m_types[i.value ()]);
  abort ();
}

void
valfile::invalidate_slot (slot_idx i)
{
  assert (m_types[i.value ()] != slot_type::END);
  assert (m_types[i.value ()] != slot_type::INVALID);
  m_slots[i.value ()].destroy (m_types[i.value ()]);
  m_types[i.value ()] = slot_type::INVALID;
}

void
valfile::set_slot (slot_idx i, value &&sv)
{
  sv.move_to_slot (i, *this);
}

void
valfile::set_slot (slot_idx i, std::unique_ptr <value> vp)
{
  vp->move_to_slot (i, *this);
}

void
valfile::set_slot (slot_idx i, constant &&cst)
{
  assert (m_types[i.value ()] != slot_type::END);
  assert (m_types[i.value ()] == slot_type::INVALID);
  m_types[i.value ()] = slot_type::CST;
  new (&m_slots[i.value ()].cst) constant { cst };
}

void
valfile::set_slot (slot_idx i, std::string &&str)
{
  assert (m_types[i.value ()] != slot_type::END);
  assert (m_types[i.value ()] == slot_type::INVALID);
  m_types[i.value ()] = slot_type::STR;
  new (&m_slots[i.value ()].str) std::string { str };
}

void
valfile::set_slot (slot_idx i, value_vector_t &&seq)
{
  assert (m_types[i.value ()] != slot_type::END);
  assert (m_types[i.value ()] == slot_type::INVALID);
  m_types[i.value ()] = slot_type::SEQ;
  new (&m_slots[i.value ()].seq) value_vector_t { std::move (seq) };
}

void
valfile::set_slot (slot_idx i, Dwarf_Die const &die)
{
  assert (m_types[i.value ()] != slot_type::END);
  assert (m_types[i.value ()] == slot_type::INVALID);
  m_types[i.value ()] = slot_type::DIE;
  m_slots[i.value ()].die = die;
}

void
valfile::set_slot (slot_idx i, attribute_slot &&attr)
{
  assert (m_types[i.value ()] != slot_type::END);
  assert (m_types[i.value ()] == slot_type::INVALID);
  m_types[i.value ()] = slot_type::ATTR;
  m_slots[i.value ()].attr = attr;
}

void
valfile::const_iterator::next ()
{
  assert (m_i != -1);
  while (m_seq->m_types[m_i] == slot_type::INVALID)
    ++m_i;

  if (m_seq->m_types[m_i] == slot_type::END)
    m_i = -1;
}

bool
valfile::const_iterator::operator== (const_iterator that) const
{
  return m_seq == that.m_seq
    && m_i == that.m_i;
}

bool
valfile::const_iterator::operator!= (const_iterator that) const
{
  return !(*this == that);
}

valfile::const_iterator &
valfile::const_iterator::operator++ ()
{
  assert (m_i != -1);
  ++m_i;
  next ();
  return *this;
}

valfile::const_iterator
valfile::const_iterator::operator++ (int)
{
  const_iterator tmp = *this;
  ++*this;
  return tmp;
}

std::unique_ptr <valueref>
valfile::const_iterator::operator* () const
{
  assert (m_i >= 0);
  return m_seq->get_slot (slot_idx (unsigned (m_i)));
}

std::unique_ptr <valueref>
valfile::const_iterator::operator-> () const
{
  return **this;
}

#if 0
int
main(int argc, char *argv[])
{
  auto stk = valfile::create (10);
  stk->set_slot (slot_idx (0), value_str { "blah" });
  stk->set_slot (slot_idx (1),
		 value_cst { constant { 17, &untyped_constant_dom } });

  {
    value_vector_t v;
    v.push_back (std::make_unique <value_str> ("blah"));
    v.push_back (std::make_unique <value_cst>
		 (constant { 17, &untyped_constant_dom }));
    stk->set_slot (slot_idx (2), value_seq { std::move (v) });
  }

  auto stk2 = valfile::copy (*stk, 10);
  for (auto it = stk2->begin (); it != stk2->end (); ++it)
    {
      auto v = *it;
      v->show (std::cout);
      std::cout << std::endl;
    }
}
#endif
