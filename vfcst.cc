#include <iostream>

#include "vfcst.hh"

static struct
  : public constant_dom
{
  void
  show (mpz_class const &v, std::ostream &o) const override
  {
    if (v.fits_uint_p ())
      switch (static_cast <slot_type_id> (v.get_ui ()))
	{
	case slot_type_id::T_CLOSURE:
	  o << "T_CLOSURE";
	  return;
	case slot_type_id::T_CONST:
	  o << "T_CONST";
	  return;
	case slot_type_id::T_FLOAT:
	  o << "T_FLOAT";
	  return;
	case slot_type_id::T_STR:
	  o << "T_STR";
	  return;
	case slot_type_id::T_SEQ:
	  o << "T_SEQ";
	  return;
	case slot_type_id::T_NODE:
	  o << "T_NODE";
	  return;
	case slot_type_id::T_ATTR:
	  o << "T_ATTR";
	  return;
	}

    assert (! "Invalid slot type constant value.");
    abort ();
  }

  std::string name () const override
  {
    return "type";
  }
} slot_type_dom_obj;

constant_dom const &slot_type_dom = slot_type_dom_obj;


numeric_constant_dom_t pos_dom_obj ("pos");
constant_dom const &pos_dom = pos_dom_obj;
