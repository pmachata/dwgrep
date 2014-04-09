#include <iostream>
#include <sstream>
#include <memory>
#include "make_unique.hh"

#include "op.hh"
#include "dwit.hh"
#include "dwpp.hh"
#include "dwcst.hh"
#include "vfcst.hh"

valfile::uptr
op_origin::next ()
{
  return std::move (m_vf);
}

std::string
op_origin::name () const
{
  return "origin";
}

void
op_origin::set_next (valfile::uptr vf)
{
  assert (m_vf == nullptr);

  // set_next should have been preceded with a reset() call that
  // should have percolated all the way here.
  assert (m_reset);
  m_reset = false;

  m_vf = std::move (vf);
}

void
op_origin::reset ()
{
  m_vf = nullptr;
  m_reset = true;
}


struct op_sel_universe::pimpl
{
  std::shared_ptr <op> m_upstream;
  std::shared_ptr <Dwarf> m_dw;
  all_dies_iterator m_it;
  valfile::uptr m_vf;
  size_t m_size;
  slot_idx m_dst;

  pimpl (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
	 size_t size, slot_idx dst)
    : m_upstream (upstream)
    , m_dw (q->dwarf)
    , m_it (all_dies_iterator::end ())
    , m_size (size)
    , m_dst (dst)
  {}

  valfile::uptr
  next ()
  {
    while (true)
      {
	if (m_vf == nullptr)
	  {
	    m_vf = m_upstream->next ();
	    if (m_vf == nullptr)
	      return nullptr;
	    m_it = all_dies_iterator (&*m_dw);
	  }

	if (m_it != all_dies_iterator::end ())
	  {
	    auto ret = valfile::copy (*m_vf, m_size);
	    ret->set_slot (m_dst, **m_it);
	    ++m_it;
	    return ret;
	  }

	m_vf = nullptr;
      }
  }

  void
  reset ()
  {
    m_vf = nullptr;
    m_upstream->reset ();
  }
};

op_sel_universe::op_sel_universe (std::shared_ptr <op> upstream,
				  dwgrep_graph::sptr q,
				  size_t size, slot_idx dst)
  : m_pimpl (std::make_unique <pimpl> (upstream, q, size, dst))
{}

op_sel_universe::~op_sel_universe ()
{}

valfile::uptr
op_sel_universe::next ()
{
  return m_pimpl->next ();
}

std::string
op_sel_universe::name () const
{
  return "sel_universe";
}

void
op_sel_universe::reset ()
{
  m_pimpl->reset ();
}

struct op_f_child::pimpl
{
  std::shared_ptr <op> m_upstream;
  valfile::uptr m_vf;
  Dwarf_Die m_child;

  size_t m_size;
  slot_idx m_src;
  slot_idx m_dst;

  pimpl (std::shared_ptr <op> upstream,
	 size_t size, slot_idx src, slot_idx dst)
    : m_upstream (upstream)
    , m_child {}
    , m_size (size)
    , m_src (src)
    , m_dst (dst)
  {}

  valfile::uptr
  next ()
  {
    while (true)
      {
	while (m_vf == nullptr)
	  {
	    if (auto vf = m_upstream->next ())
	      {
		if (vf->get_slot_type (m_src) == slot_type::DIE)
		  {
		    Dwarf_Die die = vf->get_slot_die (m_src);
		    if (dwarf_haschildren (&die))
		      {
			if (dwarf_child (&die, &m_child) != 0)
			  throw_libdw ();

			// We found our guy.
			m_vf = std::move (vf);
		      }
		  }
	      }
	    else
	      return nullptr;
	  }

	auto ret = valfile::copy (*m_vf, m_size);
	if (m_src == m_dst)
	  ret->invalidate_slot (m_dst);
	ret->set_slot (m_dst, m_child);

	switch (dwarf_siblingof (&m_child, &m_child))
	  {
	  case -1:
	    throw_libdw ();
	  case 1:
	    // No more siblings.
	    m_vf = nullptr;
	    break;
	  case 0:
	    break;
	  }

	return ret;
      }
  }

  void
  reset ()
  {
    m_vf = nullptr;
    m_upstream->reset ();
  }
};

op_f_child::op_f_child (std::shared_ptr <op> upstream,
			size_t size, slot_idx src, slot_idx dst)
  : m_pimpl (std::make_unique <pimpl> (upstream, size, src, dst))
{}

op_f_child::~op_f_child ()
{}

valfile::uptr
op_f_child::next ()
{
  return m_pimpl->next ();
}

std::string
op_f_child::name () const
{
  return "f_child";
}

void
op_f_child::reset ()
{
  m_pimpl->reset ();
}


struct op_f_attr::pimpl
{
  std::shared_ptr <op> m_upstream;
  Dwarf_Die m_die;
  valfile::uptr m_vf;
  attr_iterator m_it;

  size_t m_size;
  slot_idx m_src;
  slot_idx m_dst;

  pimpl (std::shared_ptr <op> upstream,
	 size_t size, slot_idx src, slot_idx dst)
    : m_upstream (upstream)
    , m_die {}
    , m_it (attr_iterator::end ())
    , m_size (size)
    , m_src (src)
    , m_dst (dst)
  {}

  valfile::uptr
  next ()
  {
    while (true)
      {
	while (m_vf == nullptr)
	  {
	    if (auto vf = m_upstream->next ())
	      {
		if (vf->get_slot_type (m_src) == slot_type::DIE)
		  {
		    m_die = vf->get_slot_die (m_src);
		    m_it = attr_iterator (&m_die);
		    m_vf = std::move (vf);
		  }
	      }
	    else
	      return nullptr;
	  }

	if (m_it != attr_iterator::end ())
	  {
	    auto ret = valfile::copy (*m_vf, m_size);
	    if (m_src == m_dst)
	      ret->invalidate_slot (m_dst);
	    attribute_slot slot {**m_it, dwarf_dieoffset (&m_die)};
	    ret->set_slot (m_dst, std::move (slot));
	    ++m_it;
	    return ret;
	  }

	m_vf = nullptr;
      }
  }

  void
  reset ()
  {
    m_vf = nullptr;
    m_upstream->reset ();
  }
};

op_f_attr::op_f_attr (std::shared_ptr <op> upstream,
		      size_t size, slot_idx src, slot_idx dst)
  : m_pimpl (std::make_unique <pimpl> (upstream, size, src, dst))
{}

op_f_attr::~op_f_attr ()
{}

valfile::uptr
op_f_attr::next ()
{
  return m_pimpl->next ();
}

std::string
op_f_attr::name () const
{
  return "f_attr";
}

void
op_f_attr::reset ()
{
  m_pimpl->reset ();
}


valfile::uptr
op_nop::next ()
{
  return m_upstream->next ();
}

std::string
op_nop::name () const
{
  return "nop";
}


valfile::uptr
op_assert::next ()
{
  while (auto vf = m_upstream->next ())
    if (m_pred->result (*vf) == pred_result::yes)
      return vf;
  return nullptr;
}

std::string
op_assert::name () const
{
  std::stringstream ss;
  ss << "assert<" << m_pred->name () << ">";
  return ss.str ();
}

valfile::uptr
dwop_f::next ()
{
  while (auto vf = m_upstream->next ())
    if (vf->get_slot_type (m_src) == slot_type::DIE)
      {
	Dwarf_Die die = vf->get_slot_die (m_src);
	if (m_src == m_dst)
	  vf->invalidate_slot (m_dst);
	if (operate (*vf, m_dst, die))
	  return vf;
      }
    else if (vf->get_slot_type (m_src) == slot_type::ATTR)
      {
	attribute_slot attr = vf->get_slot_attr (m_src);
	if (m_src == m_dst)
	  vf->invalidate_slot (m_dst);
	if (operate (*vf, m_dst, attr))
	  return vf;
      }

  return nullptr;
}

namespace
{
  void
  atval_unsigned_with_domain (valfile &vf, slot_idx dst, Dwarf_Attribute attr,
			      constant_dom const &dom)
  {
    Dwarf_Word uval;
    if (dwarf_formudata (&attr, &uval) != 0)
      throw_libdw ();
    constant cst { uval, &dom };
    vf.set_slot (dst, std::move (cst));
  }

  void
  atval_unsigned (valfile &vf, slot_idx dst, Dwarf_Attribute attr)
  {
    atval_unsigned_with_domain (vf, dst, attr, unsigned_constant_dom);
  }

  void
  atval_signed (valfile &vf, slot_idx dst, Dwarf_Attribute attr)
  {
    Dwarf_Sword sval;
    if (dwarf_formsdata (&attr, &sval) != 0)
      throw_libdw ();
    constant cst { (uint64_t) sval, &signed_constant_dom };
    vf.set_slot (dst, std::move (cst));
  }

  void
  handle_at_dependent_value (valfile &vf, slot_idx dst,
			     Dwarf_Attribute attr, Dwarf_Die die)
  {
    switch (dwarf_whatattr (&attr))
      {
      case DW_AT_language:
	atval_unsigned_with_domain (vf, dst, attr, dw_lang_dom);
	return;

      case DW_AT_inline:
	atval_unsigned_with_domain (vf, dst, attr, dw_inline_dom);
	return;

      case DW_AT_encoding:
	atval_unsigned_with_domain (vf, dst, attr, dw_encoding_dom);
	return;

      case DW_AT_accessibility:
	atval_unsigned_with_domain (vf, dst, attr, dw_access_dom);
	return;

      case DW_AT_visibility:
	atval_unsigned_with_domain (vf, dst, attr, dw_visibility_dom);
	return;

      case DW_AT_virtuality:
	atval_unsigned_with_domain (vf, dst, attr, dw_virtuality_dom);
	return;

      case DW_AT_identifier_case:
	atval_unsigned_with_domain (vf, dst, attr, dw_identifier_case_dom);
	return;

      case DW_AT_calling_convention:
	atval_unsigned_with_domain (vf, dst, attr,
				    dw_calling_convention_dom);
	return;

      case DW_AT_ordering:
	atval_unsigned_with_domain (vf, dst, attr, dw_ordering_dom);
	return;

      case DW_AT_decimal_sign:
	atval_unsigned_with_domain (vf, dst, attr, dw_decimal_sign_dom);
	return;

      case DW_AT_address_class:
	atval_unsigned_with_domain (vf, dst, attr, dw_address_class_dom);
	return;

      case DW_AT_endianity:
	atval_unsigned_with_domain (vf, dst, attr, dw_endianity_dom);
	return;

      case DW_AT_decl_line:
      case DW_AT_call_line:
	atval_unsigned_with_domain (vf, dst, attr, line_number_dom);
	return;

      case DW_AT_decl_column:
      case DW_AT_call_column:
	atval_unsigned_with_domain (vf, dst, attr, column_number_dom);
	return;

      case DW_AT_decl_file:
      case DW_AT_call_file:
	{
	  Dwarf_Die cudie;
	  if (dwarf_diecu (&die, &cudie, nullptr, nullptr) == nullptr)
	    throw_libdw ();

	  Dwarf_Files *files;
	  size_t nfiles;
	  if (dwarf_getsrcfiles (&cudie, &files, &nfiles) != 0)
	    throw_libdw ();

	  Dwarf_Word uval;
	  if (dwarf_formudata (&attr, &uval) != 0)
	    throw_libdw ();

	  const char *fn = dwarf_filesrc (files, uval, nullptr, nullptr);
	  if (fn == nullptr)
	    throw_libdw ();

	  vf.set_slot (dst, std::string (fn));
	  return;
	}

      case DW_AT_const_value:
	{
	  // This might be a DW_TAG_variable, DW_TAG_constant,
	  // DW_TAG_formal_parameter, DW_TAG_inlined_subroutine,
	  // DW_TAG_template_value_parameter or DW_TAG_enumerator.
	  // In the former cases, we need to determine signedness
	  // by inspecting the @type.
	  //
	  // To handle the enumerator, we need to look at @type in
	  // the parent (DW_TAG_enumeration_type).  We don't have
	  // access to that at this point.  We will have later,
	  // when "parent" op is implemented.  For now, just
	  // consider them unsigned.  NIY.

	  if (dwarf_tag (&die) != DW_TAG_enumerator)
	    {
	      Dwarf_Die type_die = die;
	      Dwarf_Attribute at;
	      while (dwarf_hasattr_integrate (&type_die, DW_AT_type))
		{
		  if (dwarf_attr_integrate (&type_die, DW_AT_type,
					    &at) == nullptr
		      || dwarf_formref_die (&at, &type_die) == nullptr)
		    throw_libdw ();
		  int tag = dwarf_tag (&type_die);
		  if (tag != DW_TAG_const_type
		      && tag != DW_TAG_volatile_type
		      && tag != DW_TAG_restrict_type
		      && tag != DW_TAG_mutable_type
		      && tag != DW_TAG_typedef
		      && tag != DW_TAG_subrange_type
		      && tag != DW_TAG_packed_type)
		    break;
		}

	      if (dwarf_tag (&type_die) != DW_TAG_base_type
		  || ! dwarf_hasattr_integrate (&type_die, DW_AT_encoding))
		{
		  // Ho hum.  This could be a structure, a pointer, or
		  // something similarly useless.
		  atval_unsigned (vf, dst, attr);
		  return;
		}
	      else
		{
		  if (dwarf_attr_integrate (&type_die, DW_AT_encoding,
					    &at) == nullptr)
		    throw_libdw ();

		  Dwarf_Word uval;
		  if (dwarf_formudata (&at, &uval) != 0)
		    throw_libdw ();

		  switch (uval)
		    {
		    case DW_ATE_signed:
		    case DW_ATE_signed_char:
		      atval_signed (vf, dst, attr);
		      return;

		    case DW_ATE_unsigned:
		    case DW_ATE_unsigned_char:
		    case DW_ATE_address:
		    case DW_ATE_boolean:
		      atval_unsigned (vf, dst, attr);
		      return;

		    case DW_ATE_float:
		    case DW_ATE_complex_float:
		    case DW_ATE_imaginary_float: // ???
		      // Encoding floating-point enumerators in
		      // DW_FORM_data* actually makes sense, but
		      // for now, NIY.
		      assert (! "float enumerator unhandled");
		      abort ();

		    case DW_ATE_signed_fixed:
		    case DW_ATE_unsigned_fixed:
		    case DW_ATE_packed_decimal:
		    case DW_ATE_decimal_float:
		      // OK, gross.
		      assert (! "weird-float enumerator unhandled");
		      abort ();

		    default:
		      // There's a couple more that nobody should
		      // probably put inside DW_FORM_data*.
		      assert (! "unknown enumerator encoding");
		      abort ();
		    }
		}
	    }
	  else
	    {
	      atval_signed (vf, dst, attr);
	      return;
	    }
	}

      case DW_AT_byte_stride:
      case DW_AT_bit_stride:
	// """Note that the stride can be negative."""
      case DW_AT_binary_scale:
      case DW_AT_decimal_scale:
	atval_signed (vf, dst, attr);
	return;

      case DW_AT_byte_size:
      case DW_AT_bit_size:
      case DW_AT_bit_offset:
      case DW_AT_data_bit_offset:
      case DW_AT_data_member_location:
      case DW_AT_lower_bound:
      case DW_AT_upper_bound:
      case DW_AT_count:
      case DW_AT_allocated:
      case DW_AT_associated:
      case DW_AT_start_scope:
      case DW_AT_digit_count:
      case DW_AT_GNU_odr_signature:
	{
	  atval_unsigned (vf, dst, attr);
	  return;
	}

      case DW_AT_discr_value:
	// ^^^ """The number is signed if the tag type for the
	// variant part containing this variant is a signed
	// type."""
	assert (! "signedness of attribute not implemented yet");
	abort ();
      }

    std::cerr << dwarf_whatattr (&attr) << std::endl;
    assert (! "signedness of attribute unhandled");
    abort ();
  }

  void
  at_value (valfile &vf, slot_idx dst, Dwarf_Attribute attr, Dwarf_Die die)
  {
    switch (dwarf_whatform (&attr))
      {
      case DW_FORM_string:
      case DW_FORM_strp:
      case DW_FORM_GNU_strp_alt:
	{
	  const char *str = dwarf_formstring (&attr);
	  if (str == nullptr)
	    throw_libdw ();
	  vf.set_slot (dst, std::string (str));
	  return;
	}

      case DW_FORM_ref_addr:
      case DW_FORM_ref1:
      case DW_FORM_ref2:
      case DW_FORM_ref4:
      case DW_FORM_ref8:
      case DW_FORM_ref_udata:
	{
	  Dwarf_Die die;
	  if (dwarf_formref_die (&attr, &die) == nullptr)
	    throw_libdw ();
	  vf.set_slot (dst, std::move (die));
	  return;
	}

      case DW_FORM_sdata:
	atval_signed (vf, dst, attr);
	return;

      case DW_FORM_udata:
	atval_unsigned (vf, dst, attr);
	return;

      case DW_FORM_addr:
	{
	  // XXX Eventually we might want to have a dedicated type
	  // that carries along information necessary for later
	  // translation into symbol-relative offsets (like what
	  // readelf does).
	  Dwarf_Addr addr;
	  if (dwarf_formaddr (&attr, &addr) != 0)
	    throw_libdw ();
	  constant cst { addr, &hex_constant_dom };
	  vf.set_slot (dst, std::move (cst));
	  return;
	}

      case DW_FORM_flag:
      case DW_FORM_flag_present:
	{
	  bool flag;
	  if (dwarf_formflag (&attr, &flag) != 0)
	    throw_libdw ();
	  constant cst { static_cast <unsigned> (flag), &bool_constant_dom };
	  vf.set_slot (dst, std::move (cst));
	  return;
	}

      case DW_FORM_data1:
      case DW_FORM_data2:
      case DW_FORM_data4:
      case DW_FORM_data8:
	handle_at_dependent_value (vf, dst, attr, die);
	return;

      case DW_FORM_block1:
      case DW_FORM_block2:
      case DW_FORM_block4:
      case DW_FORM_block:
	{
	  // XXX even for blocks, we need to look at the @type to
	  // figure out whether there's a better way to represent this
	  // item.  Some of these might be e.g. floats.  NIY.
	  Dwarf_Block block;
	  if (dwarf_formblock (&attr, &block) != 0)
	    throw_libdw ();

	  value_vector_t vv;
	  for (Dwarf_Word i = 0; i < block.length; ++i)
	    vv.push_back (std::make_unique <value_cst>
			  (constant { block.data[i], &hex_constant_dom }));
	  vf.set_slot (dst, std::move (vv));
	  return;
	}

      case DW_FORM_sec_offset:
      case DW_FORM_exprloc:
      case DW_FORM_ref_sig8:
      case DW_FORM_GNU_ref_alt:
	std::cerr << "Form unhandled: "
		  << constant (dwarf_whatform (&attr), &dw_form_dom)
		  << std::endl;
	vf.set_slot (dst, std::string ("(form unhandled)"));
	return;

      case DW_FORM_indirect:
	assert (! "Form unhandled.");
	abort  ();
      }

    assert (! "Unhandled DWARF form type.");
    abort ();
  }
}

bool
op_f_atval::operate (valfile &vf, slot_idx dst, Dwarf_Die die) const
{
  Dwarf_Attribute attr;
  if (dwarf_attr_integrate (&die, m_name, &attr) == nullptr)
    return false;

  at_value (vf, dst, attr, die);
  return true;
}

std::string
op_f_atval::name () const
{
  std::stringstream ss;
  ss << "f_atval<" << m_name << ">";
  return ss.str ();
}

bool
op_f_offset::operate (valfile &vf, slot_idx dst, Dwarf_Die die) const
{
  Dwarf_Off off = dwarf_dieoffset (&die);
  vf.set_slot (dst, constant { off, &hex_constant_dom });
  return true;
}

std::string
op_f_offset::name () const
{
  return "f_offset";
}

namespace
{
  bool
  operate_tag (valfile &vf, slot_idx dst, Dwarf_Die die)
  {
    int tag = dwarf_tag (&die);
    assert (tag >= 0);
    vf.set_slot (dst, constant { (unsigned) tag, &dw_tag_dom });
    return true;
  }
}

bool
op_f_name::operate (valfile &vf, slot_idx dst, Dwarf_Die die) const
{
  return operate_tag (vf, dst, die);
}

bool
op_f_name::operate (valfile &vf, slot_idx dst, attribute_slot attr) const
{
  unsigned name = dwarf_whatattr (&attr.attr);
  vf.set_slot (dst, constant { name, &dw_attr_dom });
  return true;
}

std::string
op_f_name::name () const
{
  return "f_name";
}

bool
op_f_tag::operate (valfile &vf, slot_idx dst, Dwarf_Die die) const
{
  return operate_tag (vf, dst, die);
}

std::string
op_f_tag::name () const
{
  return "f_tag";
}

bool
op_f_form::operate (valfile &vf, slot_idx dst, attribute_slot attr) const
{
  unsigned name = dwarf_whatform (&attr.attr);
  vf.set_slot (dst, constant { name, &dw_form_dom });
  return true;
}

std::string
op_f_form::name () const
{
  return "f_form";
}


bool
op_f_value::operate (valfile &vf, slot_idx dst, attribute_slot attr) const
{
  Dwarf_Die die;
  if (dwarf_offdie (&*m_g->dwarf, attr.die_off, &die) == nullptr)
    throw_libdw ();

  at_value (vf, dst, attr.attr, die);
  return true;
}

std::string
op_f_value::name () const
{
  return "f_value";
}


bool
op_f_parent::operate (valfile &vf, slot_idx dst, attribute_slot attr) const
{
  Dwarf_Die die;
  if (dwarf_offdie (&*m_g->dwarf, attr.die_off, &die) == nullptr)
    throw_libdw ();

  vf.set_slot (dst, die);
  return true;
}

bool
op_f_parent::operate (valfile &vf, slot_idx dst, Dwarf_Die die) const
{
  Dwarf_Off par_off = m_g->find_parent (die);
  if (par_off == dwgrep_graph::none_off)
    return false;

  if (dwarf_offdie (&*m_g->dwarf, par_off, &die) == nullptr)
    throw_libdw ();
  vf.set_slot (dst, die);
  return true;
}

std::string
op_f_parent::name () const
{
  return "f_parent";
}


void
stringer_origin::set_next (valfile::uptr vf)
{
  assert (m_vf == nullptr);

  // set_next should have been preceded with a reset() call that
  // should have percolated all the way here.
  assert (m_reset);
  m_reset = false;

  m_vf = std::move (vf);
}

std::pair <valfile::uptr, std::string>
stringer_origin::next ()
{
  return std::make_pair (std::move (m_vf), "");
}

void
stringer_origin::reset ()
{
  m_vf = nullptr;
  m_reset = true;
}

std::pair <valfile::uptr, std::string>
stringer_lit::next ()
{
  auto up = m_upstream->next ();
  if (up.first == nullptr)
    return std::make_pair (nullptr, "");
  up.second += m_str;
  return up;
}

void
stringer_lit::reset ()
{
  m_upstream->reset ();
}

std::pair <valfile::uptr, std::string>
stringer_op::next ()
{
  while (true)
    {
      if (! m_have)
	{
	  auto up = m_upstream->next ();
	  if (up.first == nullptr)
	    return std::make_pair (nullptr, "");

	  m_op->reset ();
	  m_origin->set_next (std::move (up.first));
	  m_str = up.second;

	  m_have = true;
	}

      if (auto op_vf = m_op->next ())
	{
	  std::stringstream ss;
	  op_vf->get_slot (m_src)->show (ss);
	  op_vf->invalidate_slot (m_src);
	  return std::make_pair (std::move (op_vf), m_str + ss.str ());
	}

      m_have = false;
    }
}

void
stringer_op::reset ()
{
  m_have = false;
  m_op->reset ();
  m_upstream->reset ();
}

valfile::uptr
op_format::next ()
{
  while (true)
    {
      auto s = m_stringer->next ();
      if (s.first != nullptr)
	{
	  s.first->set_slot (m_dst, std::string (s.second));
	  return std::move (s.first);
	}

      if (auto vf = m_upstream->next ())
	{
	  m_stringer->reset ();
	  m_origin->set_next (std::move (vf));
	}
      else
	return nullptr;
    }
}

void
op_format::reset ()
{
  m_stringer->reset ();
  m_upstream->reset ();
}

std::string
op_format::name () const
{
  return "format";
}

valfile::uptr
op_drop::next ()
{
  if (auto vf = m_upstream->next ())
    {
      vf->invalidate_slot (m_idx);
      return vf;
    }
  return nullptr;
}

void
op_drop::reset ()
{
  m_upstream->reset ();
}

std::string
op_drop::name () const
{
  return "drop";
}

valfile::uptr
op_dup::next ()
{
  if (auto vf = m_upstream->next ())
    {
      vf->get_slot (m_src)->clone ()->move_to_slot (m_dst, *vf);
      return vf;
    }
  return nullptr;
}

void
op_dup::reset ()
{
  m_upstream->reset ();
}

std::string
op_dup::name () const
{
  return "drop";
}


valfile::uptr
op_swap::next ()
{
  if (auto vf = m_upstream->next ())
    {
      auto tmp = vf->get_slot (m_dst_a)->clone ();
      vf->invalidate_slot (m_dst_a);
      vf->get_slot (m_dst_b)->clone ()->move_to_slot (m_dst_a, *vf);
      vf->invalidate_slot (m_dst_b);
      tmp->move_to_slot (m_dst_b, *vf);
      return vf;
    }
  return nullptr;
}

void
op_swap::reset ()
{
  m_upstream->reset ();
}

std::string
op_swap::name () const
{
  return "swap";
}


valfile::uptr
op_const::next ()
{
  if (auto vf = m_upstream->next ())
    {
      vf->set_slot (m_dst, constant (m_cst));
      return vf;
    }
  return nullptr;
}

std::string
op_const::name () const
{
  std::stringstream ss;
  ss << "const<" << m_cst << ">";
  return ss.str ();
}


valfile::uptr
op_strlit::next ()
{
  if (auto vf = m_upstream->next ())
    {
      vf->set_slot (m_dst, std::string (m_str));
      return vf;
    }
  return nullptr;
}

std::string
op_strlit::name () const
{
  std::stringstream ss;
  ss << "strlit<" << m_str << ">";
  return ss.str ();
}


valfile::uptr
op_empty_list::next ()
{
  if (auto vf = m_upstream->next ())
    {
      vf->set_slot (m_dst, value_vector_t ());
      return vf;
    }
  return nullptr;
}

std::string
op_empty_list::name () const
{
  return "empty_list";
}


valfile::uptr
op_tine::next ()
{
  if (std::all_of (m_file->begin (), m_file->end (),
		   [] (valfile::uptr const &ptr) { return ptr == nullptr; }))
    {
      if (auto vf = m_upstream->next ())
	for (auto &ptr: *m_file)
	  ptr = valfile::copy (*vf, m_size);
      else
	return nullptr;
    }

  return std::move ((*m_file)[m_branch_id]);
}

void
op_tine::reset ()
{
  for (auto &vf: *m_file)
    vf = nullptr;
  m_upstream->reset ();
}

std::string
op_tine::name () const
{
  return "tine";
}


valfile::uptr
op_merge::next ()
{
  if (m_it == m_ops.end ())
    return nullptr;

  for (size_t i = 0; i < m_ops.size (); ++i)
    {
      if (auto ret = (*m_it)->next ())
	return ret;
      if (++m_it == m_ops.end ())
	m_it = m_ops.begin ();
    }

  m_it = m_ops.end ();
  return nullptr;
}

void
op_merge::reset ()
{
  m_it = m_ops.begin ();
  for (auto op: m_ops)
    op->reset ();
}

std::string
op_merge::name () const
{
  return "merge";
}


valfile::uptr
op_capture::next ()
{
  if (auto vf = m_upstream->next ())
    {
      m_op->reset ();
      m_origin->set_next (valfile::copy (*vf, m_size));

      value_vector_t vv;
      while (auto vf2 = m_op->next ())
	vv.push_back (vf2->get_slot (m_src)->clone ());
      vf->set_slot (m_dst, std::move (vv));
      return vf;
    }
  return nullptr;
}

void
op_capture::reset ()
{
  m_op->reset ();
  m_upstream->reset ();
}

std::string
op_capture::name () const
{
  std::stringstream ss;
  ss << "capture<" << m_op->name () << ">";
  return ss.str ();
}


valfile::uptr
op_f_each::next ()
{
  while (true)
    {
      while (m_vf == nullptr)
	{
	  if (auto vf = m_upstream->next ())
	    {
	      if (vf->get_slot_type (m_src) == slot_type::SEQ)
		{
		  m_i = 0;
		  m_vf = std::move (vf);
		}
	    }
	  else
	    return nullptr;
	}

      value_vector_t const &vv = m_vf->get_slot_seq (m_src);
      if (m_i < vv.size ())
	{
	  std::unique_ptr <value> v = vv[m_i++]->clone ();
	  valfile::uptr vf = valfile::copy (*m_vf, m_size);
	  if (m_src == m_dst)
	    vf->invalidate_slot (m_dst);
	  v->move_to_slot (m_dst, *vf);
	  return vf;
	}

      m_vf = nullptr;
    }
}

void
op_f_each::reset ()
{
  m_vf = nullptr;
  m_upstream->reset ();
}

std::string
op_f_each::name () const
{
  return "f_each";
}


valfile::uptr
op_f_type::next ()
{
  if (auto vf = m_upstream->next ())
    {
      slot_type t = vf->get_slot_type (m_src);
      if (m_src == m_dst)
	vf->invalidate_slot (m_dst);

      switch (t)
	{
	case slot_type::END:
	case slot_type::INVALID:
	  assert (! "Invalid slot type.");
	  abort ();

	case slot_type::CST:
	  vf->set_slot (m_dst, constant {(int) slot_type_id::T_CONST,
		&slot_type_dom});
	  return vf;

	case slot_type::FLT:
	  vf->set_slot (m_dst, constant {(int) slot_type_id::T_FLOAT,
		&slot_type_dom});
	  return vf;

	case slot_type::SEQ:
	  vf->set_slot (m_dst, constant {(int) slot_type_id::T_SEQ,
		&slot_type_dom});
	  return vf;

	case slot_type::STR:
	  vf->set_slot (m_dst, constant {(int) slot_type_id::T_STR,
		&slot_type_dom});
	  return vf;

	case slot_type::DIE:
	case slot_type::LINE:
	case slot_type::LOCLIST_ENTRY:
	case slot_type::LOCLIST_OP:
	  vf->set_slot (m_dst, constant {(int) slot_type_id::T_NODE,
		&slot_type_dom});
	  return vf;

	case slot_type::ATTR:
	  vf->set_slot (m_dst, constant {(int) slot_type_id::T_ATTR,
		&slot_type_dom});
	  return vf;
	}
    }

  return nullptr;
}

void
op_f_type::reset ()
{
  m_upstream->reset ();
}

std::string
op_f_type::name () const
{
  return "f_type";
}


pred_result
pred_not::result (valfile &vf)
{
  return ! m_a->result (vf);
}

std::string
pred_not::name () const
{
  std::stringstream ss;
  ss << "not <" << m_a->name () << ">";
  return ss.str ();
}


pred_result
pred_and::result (valfile &vf)
{
  return m_a->result (vf) && m_b->result (vf);
}

std::string
pred_and::name () const
{
  std::stringstream ss;
  ss << "and <" << m_a->name () << "><" << m_b->name () << ">";
  return ss.str ();
}


pred_result
pred_or::result (valfile &vf)
{
  return m_a->result (vf) || m_b->result (vf);
}

std::string
pred_or::name () const
{
  std::stringstream ss;
  ss << "or <" << m_a->name () << "><" << m_b->name () << ">";
  return ss.str ();
}

pred_result
pred_at::result (valfile &vf)
{
  if (vf.get_slot_type (m_idx) == slot_type::DIE)
    {
      auto &die = const_cast <Dwarf_Die &> (vf.get_slot_die (m_idx));
      return pred_result (dwarf_hasattr_integrate (&die, m_atname) != 0);
    }
  else if (vf.get_slot_type (m_idx) == slot_type::ATTR)
    {
      auto &attr = const_cast <Dwarf_Attribute &>
	(vf.get_slot_attr (m_idx).attr);
      return pred_result (dwarf_whatattr (&attr) == m_atname);
    }
  else
    return pred_result::fail;
}

std::string
pred_at::name () const
{
  std::stringstream ss;
  ss << "pred_at<" << m_atname << ">";
  return ss.str ();
}

pred_result
pred_tag::result (valfile &vf)
{
  if (vf.get_slot_type (m_idx) == slot_type::DIE)
    {
      auto &die = const_cast <Dwarf_Die &> (vf.get_slot_die (m_idx));
      return pred_result (dwarf_tag (&die) == m_tag);
    }
  else
    return pred_result::fail;
}

std::string
pred_tag::name () const
{
  std::stringstream ss;
  ss << "pred_tag<" << m_tag << ">";
  return ss.str ();
}

namespace
{
  template <class CMP>
  pred_result
  comparison_result (valfile &vf, slot_idx idx_a, slot_idx idx_b)
  {
    slot_type st = vf.get_slot_type (idx_a);
    if (st != vf.get_slot_type (idx_b))
      return pred_result::fail;

    switch (st)
      {
      case slot_type::END:
      case slot_type::INVALID:
	assert (! "invalid slots for comparison");
	abort ();

      case slot_type::CST:
	return CMP::result (vf.get_slot_cst (idx_a), vf.get_slot_cst (idx_b));

      case slot_type::STR:
	return CMP::result (vf.get_slot_str (idx_a), vf.get_slot_str (idx_b));

      case slot_type::FLT:
      case slot_type::SEQ:
      case slot_type::DIE:
      case slot_type::ATTR:
      case slot_type::LINE:
      case slot_type::LOCLIST_ENTRY:
      case slot_type::LOCLIST_OP:
	assert (! "NYI");
	abort ();
      }

    assert (! "invalid slot type");
    abort ();
  }

  struct cmp_eq
  {
    template <class T>
    static pred_result
    result (T const &a, T const &b)
    {
      return pred_result (a == b);
    }
  };

  struct cmp_lt
  {
    template <class T>
    static pred_result
    result (T const &a, T const &b)
    {
      return pred_result (a < b);
    }
  };

  struct cmp_gt
  {
    template <class T>
    static pred_result
    result (T const &a, T const &b)
    {
      return pred_result (a > b);
    }
  };
}

pred_result
pred_eq::result (valfile &vf)
{
  return comparison_result <cmp_eq> (vf, m_idx_a, m_idx_b);
}

std::string
pred_eq::name () const
{
  return "pred_eq";
}

pred_result
pred_lt::result (valfile &vf)
{
  return comparison_result <cmp_lt> (vf, m_idx_a, m_idx_b);
}

std::string
pred_lt::name () const
{
  return "pred_lt";
}

pred_result
pred_gt::result (valfile &vf)
{
  return comparison_result <cmp_gt> (vf, m_idx_a, m_idx_b);
}

std::string
pred_gt::name () const
{
  return "pred_gt";
}

pred_root::pred_root (dwgrep_graph::sptr g, slot_idx idx_a)
  : m_g (g)
  , m_idx_a (idx_a)
{
}

pred_result
pred_root::result (valfile &vf)
{
  if (vf.get_slot_type (m_idx_a) == slot_type::DIE)
    return pred_result (m_g->is_root (vf.get_slot_die (m_idx_a)));

  else if (vf.get_slot_type (m_idx_a) == slot_type::ATTR)
    // By definition, attributes are never root.
    return pred_result::no;

  else
    return pred_result::fail;
}

std::string
pred_root::name () const
{
  return "pred_root";
}

pred_result
pred_subx_any::result (valfile &vf)
{
  m_op->reset ();
  m_origin->set_next (valfile::copy (vf, m_size));
  if (m_op->next () != nullptr)
    return pred_result::yes;
  else
    return pred_result::no;
}

std::string
pred_subx_any::name () const
{
  std::stringstream ss;
  ss << "pred_subx_any<" << m_op->name () << ">";
  return ss.str ();
}

void
pred_subx_any::reset ()
{
  m_op->reset ();
}

pred_result
pred_empty::result (valfile &vf)
{
  switch (vf.get_slot_type (m_idx_a))
    {
    case slot_type::END:
    case slot_type::INVALID:
      assert (! "pred_empty: invalid predicate slot");
      abort ();

    case slot_type::CST:
    case slot_type::FLT:
    case slot_type::DIE:
    case slot_type::ATTR:
    case slot_type::LINE:
    case slot_type::LOCLIST_ENTRY:
    case slot_type::LOCLIST_OP:
      return pred_result::fail;

    case slot_type::STR:
      return pred_result (vf.get_slot_str (m_idx_a) == "");

    case slot_type::SEQ:
      return pred_result (vf.get_slot_seq (m_idx_a).empty ());
    }

  assert (! "pred_empty: unhandled slot type");
  abort ();
}

std::string
pred_empty::name () const
{
  return "pred_empty";
}
