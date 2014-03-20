#include "valfile.hh"

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
