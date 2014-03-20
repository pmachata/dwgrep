#include <iostream>

#include "valfile.hh"

void
std::default_delete <valfile>::operator() (valfile *ptr) const
{
  ptr->~valfile ();
  delete[] (unsigned char *) ptr;
}

void
valueref_cst::show (std::ostream &o) const
{
  o << m_cst;
}

slot_type
valueref_cst::set_slot (valfile_slot &slt) const
{
  new (&slt.cst) constant { m_cst };
  return slot_type::CST;
}

void
valueref_str::show (std::ostream &o) const
{
  o << m_str;
}

slot_type
valueref_str::set_slot (valfile_slot &slt) const
{
  new (&slt.str) std::string { m_str };
  return slot_type::STR;
}

void
valueref_seq::show (std::ostream &o) const
{
  o << "[";
  bool seen = false;
  for (auto const &v: m_seq)
    {
      if (seen)
	o << ", ";
      seen = true;
      v->show (o);
    }
  o << "]";
}

slot_type
valueref_seq::set_slot (valfile_slot &slt) const
{
  value_vector_t copy;
  for (auto const &v: m_seq)
    // XXX could we avoid this copy?  Ideally we would just move
    // things in from the outside.  Maybe have instead of valueref a
    // pure behavior class, and pass the reference from outside.  The
    // behavior class would be enclosed in either a reference wrapper,
    // or a value wrapper.
    copy.push_back (std::unique_ptr <value> { v->clone () });
  new (&slt.seq) value_vector_t { std::move (copy) };
  return slot_type::SEQ;
}

void
valfile_slot::destroy (slot_type t)
{
  switch (t)
    {
    case slot_type::INVALID:
    case slot_type::FLT:
    case slot_type::DIE:
    case slot_type::ATTRIBUTE:
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
  return { buf, reinterpret_cast <slot_type *> (buf + sz1) };
}

std::unique_ptr <valfile>
valfile::create (size_t n)
{
  allocd_block b = alloc (n);

  for (size_t i = 0; i < n; ++i)
    b.types[i] = slot_type::INVALID;
  b.types[n] = slot_type::END;

  return std::unique_ptr <valfile> { new (b.buf) valfile (b.types) };
}

std::unique_ptr <valfile>
valfile::copy (valfile const &that, size_t n)
{
  assert (that.m_types[n] == slot_type::END);
  allocd_block b = alloc (n);

  for (size_t i = 0; i < n + 1; ++i)
    b.types[i] = that.m_types[i];

  return std::unique_ptr <valfile> { new (b.buf) valfile (b.types) };
}

valfile::~valfile ()
{
  for (size_t i = 0; m_types[i] != slot_type::END; ++i)
    m_slots[i].destroy (m_types[i]);
}

slot_type
valfile::get_slot_type (size_t i) const
{
  assert (m_types[i] != slot_type::END);
  assert (m_types[i] != slot_type::INVALID);
  return m_types[i];
}

std::string const &
valfile::get_slot_str (size_t i) const
{
  assert (m_types[i] == slot_type::STR);
  return m_slots[i].str;
}

constant const &
valfile::get_slot_cst (size_t i) const
{
  assert (m_types[i] == slot_type::CST);
  return m_slots[i].cst;
}

value_vector_t const &
valfile::get_slot_seq (size_t i) const
{
  assert (m_types[i] == slot_type::SEQ);
  return m_slots[i].seq;
}

// General accessor for all slot types.
std::unique_ptr <valueref>
valfile::get_slot (size_t i) const
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
valfile::invalidate_slot (size_t i)
{
  assert (m_types[i] != slot_type::END);
  assert (m_types[i] != slot_type::INVALID);
  m_slots[i].destroy (m_types[i]);
  m_types[i] = slot_type::INVALID;
}

void
valfile::set_slot (size_t i, valueref const &sv)
{
  assert (m_types[i] == slot_type::INVALID);
  m_types[i] = sv.set_slot (m_slots[i]);
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
  assert (m_i != -1);
  return m_seq->get_slot (m_i);
}

std::unique_ptr <valueref>
valfile::const_iterator::operator-> () const
{
  return **this;
}


int main(int argc, char *argv[])
{
  auto stk = valfile::create (10);
  stk->set_slot (0, valueref_str { "blah" });
  stk->set_slot (1, valueref_cst { constant { 17, &untyped_constant_dom } });

  {
    value_vector_t v;
    v.push_back (std::unique_ptr <value> { new value_str { "blah" } });
    v.push_back (std::unique_ptr <value>
		 { new value_cst { constant { 17, &untyped_constant_dom } } });
    stk->set_slot (2, valueref_seq { v });
  }

  for (auto it = stk->begin (); it != stk->end (); ++it)
    {
      auto v = *it;
      v->show (std::cout);
      std::cout << std::endl;
    }

  //auto stk2 = seq::copy (*stk, 10);
  //return sizeof (stk);
}
