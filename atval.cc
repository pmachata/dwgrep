#include <iostream>
#include <dwarf.h>
#include <memory>
#include "make_unique.hh"

#include "atval.hh"
#include "dwcst.hh"
#include "dwpp.hh"
#include "valfile.hh"
#include "value-dw.hh"
#include "value-seq.hh"
#include "value-str.hh"

namespace
{
  std::unique_ptr <value>
  atval_unsigned_with_domain (Dwarf_Attribute attr, constant_dom const &dom)
  {
    Dwarf_Word uval;
    if (dwarf_formudata (&attr, &uval) != 0)
      throw_libdw ();
    return std::make_unique <value_cst> (constant {uval, &dom}, 0);
  }

  std::unique_ptr <value>
  atval_unsigned (Dwarf_Attribute attr)
  {
    return atval_unsigned_with_domain (attr, dec_constant_dom);
  }

  std::unique_ptr <value>
  atval_signed (Dwarf_Attribute attr)
  {
    Dwarf_Sword sval;
    if (dwarf_formsdata (&attr, &sval) != 0)
      throw_libdw ();
    return std::make_unique <value_cst> (constant {sval, &dec_constant_dom}, 0);
  }

  std::unique_ptr <value>
  atval_addr (Dwarf_Attribute attr)
  {
    // XXX Eventually we might want to have a dedicated type that
    // carries along information necessary for later translation into
    // symbol-relative offsets (like what readelf does).  NIY
    Dwarf_Addr addr;
    if (dwarf_formaddr (&attr, &addr) != 0)
      throw_libdw ();
    return std::make_unique <value_cst> (constant {addr, &hex_constant_dom}, 0);
  }

  std::unique_ptr <value>
  handle_at_dependent_value (Dwarf_Attribute attr, Dwarf_Die die,
			     dwgrep_graph &gr)
  {
    switch (dwarf_whatattr (&attr))
      {
      case DW_AT_language:
	return atval_unsigned_with_domain (attr, dw_lang_dom);

      case DW_AT_inline:
	return atval_unsigned_with_domain (attr, dw_inline_dom);

      case DW_AT_encoding:
	return atval_unsigned_with_domain (attr, dw_encoding_dom);

      case DW_AT_accessibility:
	return atval_unsigned_with_domain (attr, dw_access_dom);

      case DW_AT_visibility:
	return atval_unsigned_with_domain (attr, dw_visibility_dom);

      case DW_AT_virtuality:
	return atval_unsigned_with_domain (attr, dw_virtuality_dom);

      case DW_AT_identifier_case:
	return atval_unsigned_with_domain (attr, dw_identifier_case_dom);

      case DW_AT_calling_convention:
	return atval_unsigned_with_domain (attr, dw_calling_convention_dom);

      case DW_AT_ordering:
	return atval_unsigned_with_domain (attr, dw_ordering_dom);

      case DW_AT_decimal_sign:
	return atval_unsigned_with_domain (attr, dw_decimal_sign_dom);

      case DW_AT_address_class:
	return atval_unsigned_with_domain (attr, dw_address_class_dom);

      case DW_AT_endianity:
	return atval_unsigned_with_domain (attr, dw_endianity_dom);

      case DW_AT_decl_line:
      case DW_AT_call_line:
	return atval_unsigned_with_domain (attr, line_number_dom);

      case DW_AT_decl_column:
      case DW_AT_call_column:
	return atval_unsigned_with_domain (attr, column_number_dom);

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

	  return std::make_unique <value_str> (fn, 0);
	}

      case DW_AT_const_value:
	{
	  // This might be a DW_TAG_variable, DW_TAG_constant,
	  // DW_TAG_formal_parameter, DW_TAG_inlined_subroutine,
	  // DW_TAG_template_value_parameter or DW_TAG_enumerator.
	  // In the former cases, we need to determine signedness by
	  // inspecting the @type.  To handle the enumerator, we need
	  // to look at @type in the parent (DW_TAG_enumeration_type).

	  Dwarf_Die type_die;
	  if (dwarf_tag (&die) != DW_TAG_enumerator)
	    type_die = die;
	  else
	    {
	      // Get DW_TAG_enumeration_type.
	      if (dwarf_offdie (&*gr.dwarf, gr.find_parent (die),
				&type_die) == nullptr)
		throw_libdw ();

	      if (dwarf_tag (&type_die) != DW_TAG_enumeration_type)
		{
		  std::cerr << "Unexpected: DW_TAG_enumerator's parent is "
		    "not DW_TAG_enumeration_type\n";
		  return atval_unsigned (attr);
		}

	      if (! dwarf_hasattr_integrate (&type_die, DW_AT_type))
		{
		  std::cerr << "Unexpected: DW_TAG_enumeration_type whose "
		    "DW_TAG_enumerator's DW_AT_const_value is "
		    "DW_FORM_data[1248] doesn't have DW_AT_type.\n";
		  return atval_unsigned (attr);
		}
	    }

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
	      return atval_unsigned (attr);
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
		  return atval_signed (attr);

		case DW_ATE_unsigned:
		case DW_ATE_unsigned_char:
		case DW_ATE_address:
		case DW_ATE_boolean:
		  return atval_unsigned (attr);

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

      case DW_AT_byte_stride:
      case DW_AT_bit_stride:
	// """Note that the stride can be negative."""
      case DW_AT_binary_scale:
      case DW_AT_decimal_scale:
	return atval_signed (attr);

      case DW_AT_high_pc:
      case DW_AT_entry_pc:
	// High PC and entry PC that aren't DW_FORM_addr are relative
	// to DW_AT_low_pc (or the first DW_AT_ranges address).  But
	// we should probably keep it untranslated to keep close to
	// actual data.
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
	return atval_unsigned (attr);

      case DW_AT_stmt_list:
	return atval_unsigned_with_domain (attr, hex_constant_dom);

      case DW_AT_location:
	std::cerr << "location lists NIY\n";
	return atval_unsigned_with_domain (attr, hex_constant_dom);

      case DW_AT_ranges:
	std::cerr << "address ranges NIY\n";
	return atval_unsigned_with_domain (attr, hex_constant_dom);

      case DW_AT_discr_value:
	// ^^^ """The number is signed if the tag type for the
	// variant part containing this variant is a signed
	// type."""
	assert (! "signedness of attribute not implemented yet");
	abort ();
      }

    std::cerr << dwarf_whatattr (&attr) << std::endl << std::flush;
    assert (! "signedness of attribute unhandled");
    abort ();
  }
}

std::unique_ptr <value>
at_value (Dwarf_Attribute attr, Dwarf_Die die, dwgrep_graph::sptr gr)
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
	return std::make_unique <value_str> (str, 0);
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
	return std::make_unique <value_die> (gr, die, 0);
      }

    case DW_FORM_sdata:
      return atval_signed (attr);

    case DW_FORM_udata:
      return atval_unsigned (attr);

    case DW_FORM_addr:
      return atval_addr (attr);

    case DW_FORM_flag:
    case DW_FORM_flag_present:
      {
	bool flag;
	if (dwarf_formflag (&attr, &flag) != 0)
	  throw_libdw ();
	return std::make_unique <value_cst>
	  (constant {static_cast <unsigned> (flag), &bool_constant_dom}, 0);
      }

    case DW_FORM_data1:
    case DW_FORM_data2:
    case DW_FORM_data4:
    case DW_FORM_data8:
      return handle_at_dependent_value (attr, die, *gr);

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

	value_seq::seq_t vv;
	for (Dwarf_Word i = 0; i < block.length; ++i)
	  vv.push_back (std::make_unique <value_cst>
			(constant { block.data[i], &hex_constant_dom }, 0));
	return std::make_unique <value_seq> (std::move (vv), 0);
      }

    case DW_FORM_sec_offset:
      return atval_unsigned_with_domain (attr, hex_constant_dom);

    case DW_FORM_exprloc:
    case DW_FORM_ref_sig8:
    case DW_FORM_GNU_ref_alt:
      std::cerr << "Form unhandled: "
		<< constant (dwarf_whatform (&attr), &dw_form_dom)
		<< std::endl;
      return std::make_unique <value_str> ("(form unhandled)", 0);

    case DW_FORM_indirect:
      assert (! "Form unhandled.");
      abort  ();
    }

  assert (! "Unhandled DWARF form type.");
  abort ();
}
