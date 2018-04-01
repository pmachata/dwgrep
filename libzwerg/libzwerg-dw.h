/*
  Copyright (C) 2018 Petr Machata
  Copyright (C) 2014, 2015 Red Hat, Inc.

  This file is free software; you can redistribute it and/or modify
  it under the terms of either

  * the GNU Lesser General Public License as published by the Free
  Software Foundation; either version 3 of the License, or (at
  your option) any later version

  or

  * the GNU General Public License as published by the Free
  Software Foundation; either version 2 of the License, or (at
  your option) any later version

  or both in parallel, as here.

  dwgrep is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received copies of the GNU General Public License and
  the GNU Lesser General Public License along with this program.  If
  not, see <http://www.gnu.org/licenses/>.  */

#ifndef LIBZWERG_DW_H_
#define LIBZWERG_DW_H_

#include <libzwerg.h>
#include <libzwerg-mach.h>
#include <elfutils/libdwfl.h>

#ifdef __cplusplus
extern "C" {
#endif

  /**
   * Constant domains.
   */

  // Return a domain representing a family of DW_TAG_ constants.
  zw_cdom const *zw_cdom_dw_tag (void);

  // Return a domain representing a family of DW_AT_ constants.
  zw_cdom const *zw_cdom_dw_attr (void);

  // Return a domain representing a family of DW_FORM_ constants.
  zw_cdom const *zw_cdom_dw_form (void);

  // Return a domain representing a family of DW_LANG_ constants.
  zw_cdom const *zw_cdom_dw_lang (void);

  // Return a domain representing a family of DW_MACINFO_ constants.
  zw_cdom const *zw_cdom_dw_macinfo (void);

  // Return a domain representing a family of DW_MACRO_ constants.
  zw_cdom const *zw_cdom_dw_macro (void);

  // Return a domain representing a family of DW_INL_ constants.
  zw_cdom const *zw_cdom_dw_inline (void);

  // Return a domain representing a family of DW_ATE_ constants.
  zw_cdom const *zw_cdom_dw_encoding (void);

  // Return a domain representing a family of DW_ACCESS_ constants.
  zw_cdom const *zw_cdom_dw_access (void);

  // Return a domain representing a family of DW_VIS_ constants.
  zw_cdom const *zw_cdom_dw_visibility (void);

  // Return a domain representing a family of DW_VIRTUALITY_ constants.
  zw_cdom const *zw_cdom_dw_virtuality (void);

  // Return a domain representing a family of DW_ID_ constants.
  zw_cdom const *zw_cdom_dw_identifier_case (void);

  // Return a domain representing a family of DW_CC_ constants.
  zw_cdom const *zw_cdom_dw_calling_convention (void);

  // Return a domain representing a family of DW_ORD_ constants.
  zw_cdom const *zw_cdom_dw_ordering (void);

  // Return a domain representing a family of DW_DSC_ constants.
  zw_cdom const *zw_cdom_dw_discr_list (void);

  // Return a domain representing a family of DW_DS_ constants.
  zw_cdom const *zw_cdom_dw_decimal_sign (void);

  // Return a domain representing a family of DW_OP_ constants.
  zw_cdom const *zw_cdom_dw_locexpr_opcode (void);

  // Return a domain representing a family of DW_ADDR_ constants.
  zw_cdom const *zw_cdom_dw_address_class (void);

  // Return a domain representing a family of DW_END_ constants.
  zw_cdom const *zw_cdom_dw_endianity (void);

  // Return a domain representing a family of DW_DEFAULTED_ constants.
  zw_cdom const *zw_cdom_dw_defaulted (void);

  // Return a domain representing a family of STT_ constants.
  //
  // This is not a single domain, as some STT_ constants are
  // arch-dependent.  Two identical constants compare equal even if
  // they come from two different STT_ domains, only the arch-specific
  // ones would compare unequal even though they have the same
  // constant value (say, STT_SPARC_REGISTER and STT_ARM_TFUNC would
  // compare unequal even though they both are represented by a
  // constant 13).
  zw_cdom const *zw_cdom_elfsym_stt (zw_machine const *machine);

  // Return a domain representing a family of STB_ constants.  Much
  // like zw_cdom_elfsym_stt, this is a family of domains.
  zw_cdom const *zw_cdom_elfsym_stb (zw_machine const *machine);

  // Return a domain representing a family of STV_ constants.
  zw_cdom const *zw_cdom_elfsym_stv (void);

  /**
   * Dwarf.
   */

  // Create a new value representing possibly several DWARF (ELF)
  // values, all reported as modules at given DWFL.  NAME is a name
  // that represents the DWFL.  For a single-module DWFL it would be
  // the file name of the single module, otherwise it's some other
  // sort of identifier that's used when displaying the value.  The
  // produced value will have a position of POS.  Returns NULL on
  // error, in which case it sets *OUT_ERR.  OUT_ERR shall be
  // non-NULL.
  zw_value *zw_value_init_dwarf_dwfl (Dwfl *dwfl, char const *name,
				      size_t pos, zw_error **out_err);

  // Like zw_value_init_dwarf_dwfl, but produces a raw value.  See
  // "Dwarf vocabulary" in documentation for explanation of
  // differences between cooked and raw values.
  zw_value *zw_value_init_dwarf_dwfl_raw (Dwfl *dwfl, char const *name,
					  size_t pos, zw_error **out_err);

  // Create a new value representing a DWARF (or ELF) cooked file.
  // This is like creating a new Dwfl object, reporting a single
  // module from FILENAME, and then calling zw_value_init_dwarf_dwfl.
  zw_value *zw_value_init_dwarf (char const *filename,
				 size_t pos, zw_error **out_err);

  // Like zw_value_init_dwarf, but produces a raw value.
  zw_value *zw_value_init_dwarf_raw (char const *filename,
				     size_t pos, zw_error **out_err);

  // Return whether VAL is a DWARF (ELF) value.
  bool zw_value_is_dwarf (zw_value const *val);

  // Return a Dwfl handle associated with DW, which shall be a DWARF
  // (ELF) value.
  Dwfl *zw_value_dwarf_dwfl (zw_value const *dw);

  // Return a name associated with DW, which shall be a DWARF (ELF)
  // value.
  char const *zw_value_dwarf_name (zw_value const *dw);

  // Return a machine that modules in DW come from.  Returns NULL on
  // error, in which case it sets *OUT_ERR.  OUT_ERR shall be
  // non-NULL.
  zw_machine const *zw_value_dwarf_machine (zw_value const *dw,
					    zw_error **out_err);


  /**
   * CU.
   */

  // Return whether VAL is a CU value (meaning it represents a
  // compilation unit or other type of unit).
  bool zw_value_is_cu (zw_value const *val);

  // Return a Dwarf_CU handle representing the compilation unit
  // referenced by CU, which shall be a CU value.
  Dwarf_CU *zw_value_cu_cu (zw_value const *cu);

  // Return an offset of a CU DIE in CU, which shall be a CU value.
  Dwarf_Off zw_value_cu_offset (zw_value const *cu);


  /**
   * DIE.
   */

  // Create a DIE value, a value that represents a given DIE, which
  // comes from DW.  If IMPORT_DIE is non-NULL, it represents import
  // point of current DIE.  The IMPORT_DIE itself could have been
  // created as a DIE value with its own import point.  That's useful
  // for representing a path through DW_TAG_imported_unit that was
  // traversed to get to a DIE, which likely resides in a partial or
  // type unit.  The created value will have a position of POS.
  // Returns NULL on error, in which case it sets *OUT_ERR.  OUT_ERR
  // shall be non-NULL.
  zw_value *zw_value_init_die (zw_value const *dw, Dwarf_Die die,
			       zw_value const *import_die,
			       size_t pos, zw_error **out_err);

  // Like zw_value_init_die, but creates a raw value (and hence has no
  // import point).  See "Dwarf vocabulary" in documentation for
  // explanation of differences between cooked and raw values.
  zw_value *zw_value_init_die_raw (zw_value const *dw, Dwarf_Die die,
				   size_t pos, zw_error **out_err);

  // Return whether VAL is a DIE value.
  bool zw_value_is_die (zw_value const *val);

  // Return Dwarf_Die referenced by this value, which shall be a DIE
  // value.
  Dwarf_Die zw_value_die_die (zw_value const *die);

  // Return a DWARF (ELF) value that DIE, which shall be a DIE value,
  // comes from.  Returns NULL on error, in which case it sets
  // *OUT_ERR.  OUT_ERR shall be non-NULL.
  zw_value const *zw_value_die_dwarf (zw_value const *die, zw_error **out_err);


  /**
   * DIE attribute.
   */

  // Return whether VAL is an attribute value, meaning that it
  // references a DIE attribute.
  bool zw_value_is_attr (zw_value const *val);

  // Return an attribute that ATTR, which shall be an attribute value,
  // references.
  Dwarf_Attribute zw_value_attr_attr (zw_value const *attr);

  // Return a DWARF (ELF) value that ATTR, which shall be an attribute
  // value, comes from.  Returns NULL on error, in which case it sets
  // *OUT_ERR.  OUT_ERR shall be non-NULL.
  zw_value const *zw_value_attr_dwarf (zw_value const *attr,
				       zw_error **out_err);


  /**
   * Location list element.
   */

  // Return whether VAL is a location list value, meaning that it
  // references a particular location list.
  bool zw_value_is_llelem (zw_value const *val);

  // Return an attribute associated with LLELEM, which shall be a
  // location list value.
  Dwarf_Attribute zw_value_llelem_attribute (zw_value const *llelem);

  // Return a start of range of addresses covered by location list
  // expression held by LLELEM, which shall be a location list value.
  Dwarf_Addr zw_value_llelem_low (zw_value const *llelem);

  // Like zw_value_llelem_low, but returns end of range, which is
  // exclusive (i.e. not itself covered).  N.B.: ranges that span
  // across the whole address space are represented by location list
  // values whose low is 0, and high is 0xffffffffffffffff.
  Dwarf_Addr zw_value_llelem_high (zw_value const *llelem);

  // Return an expression referenced by LLELEM, which shall be a
  // location list value.  *OUT_LENGTH is set to number of operations
  // in the returned array.
  Dwarf_Op *zw_value_llelem_expr (zw_value const *llelem, size_t *out_length);


  /**
   * Location list operation.
   */

  // Return whether VAL is a location list operation (that is, one
  // Dwarf_Op).
  bool zw_value_is_llop (zw_value const *val);

  // Return attribute associated with LLOP, which shall be a location
  // list operation value.
  Dwarf_Attribute zw_value_llop_attribute (zw_value const *llop);

  // Return an operation that LLOP, which shall be a location list
  // operation value, references.
  Dwarf_Op *zw_value_llop_op (zw_value const *llop);


  /**
   * Address sets.
   */

  // Return whether VAL is an address set.
  bool zw_value_is_aset (zw_value const *val);

  // Return number of start-length pairs in ASET, which shall be an
  // address set value.
  size_t zw_value_aset_length (zw_value const *aset);

  // This structure describes one run of contiguous addresses in an
  // address set.
  struct zw_aset_pair
  {
    Dwarf_Addr start;
    Dwarf_Word length;
  };

  // Return IDX-th run of contiguous addresses in ASET, which shall be
  // an address set value.  IDX shall be smaller than length of
  // address set.
  struct zw_aset_pair zw_value_aset_at (zw_value const *aset, size_t idx);


  /**
   * ELF files.
   */

  // Return whether VAL is an ELF value.
  bool zw_value_is_elf (zw_value const *val);

  // Return a DWARF (ELF) value referencing the file that ELF, which
  // shall be an ELF value, comes from. Returns NULL on error, in
  // which case it sets *OUT_ERR. OUT_ERR shall be non-NULL.
  zw_value const *zw_value_elf_dwarf (zw_value const *elf,
				      zw_error **out_err);


  /**
   * ELF symbols.
   */

  // Return whether VAL is an ELF symbol value.
  bool zw_value_is_elfsym (zw_value const *val);

  // Return symbol table index of a given ELFSYM, which shall be an
  // ELF symbol value.
  unsigned zw_value_elfsym_symidx (zw_value const *elfsym);

  // Return name of ELFSYM, which shall be an ELF symbol value.
  char const *zw_value_elfsym_name (zw_value const *elfsym);

  // Return the symbol itself that's referenced by ELFSYM, which shall
  // be an ELF symbol value.
  GElf_Sym zw_value_elfsym_symbol (zw_value const *elfsym);

  // Return an ELF file handle that ELFSYM, which shall be an ELF
  // symbol value, comes from.
  Elf *zw_value_elfsym_elf (zw_value const *elfsym);

  // Return a DWARF (ELF) value referencing the file that ELFSYM,
  // which shall be an ELF symbol value, comes from.  Returns NULL on
  // error, in which case it sets *OUT_ERR.  OUT_ERR shall be
  // non-NULL.
  zw_value const *zw_value_elfsym_dwarf (zw_value const *elfsym,
					 zw_error **out_err);


#ifdef __cplusplus
}
#endif

#endif
