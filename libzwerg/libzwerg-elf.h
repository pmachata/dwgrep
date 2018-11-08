/*
  Copyright (C) 2018 Petr Machata

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

#ifndef LIBZWERG_ELF_H_
#define LIBZWERG_ELF_H_

#include <libzwerg.h>
#include <libzwerg-mach.h>

#ifdef __cplusplus
extern "C" {
#endif

  /**
   * Constant domains.
   */

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


  /**
   * ELF sections.
   */

  // Return whether VAL is an ELF section value.
  bool zw_value_is_elfscn (zw_value const *val);

#ifdef __cplusplus
}
#endif

#endif
