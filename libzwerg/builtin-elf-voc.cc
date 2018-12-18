/*
   Copyright (C) 2018 Petr Machata
   This file is part of dwgrep.

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

#include <elf.h>

#include "builtin-elf-voc.hh"
#include "builtin-elf-voc-abiversion.hh"
#include "builtin-elf-voc-address.hh"
#include "builtin-elf-voc-alignment.hh"
#include "builtin-elf-voc-binding.hh"
#include "builtin-elf-voc-eclass.hh"
#include "builtin-elf-voc-edata.hh"
#include "builtin-elf-voc-eentry.hh"
#include "builtin-elf-voc-eident.hh"
#include "builtin-elf-voc-elf.hh"
#include "builtin-elf-voc-emachine.hh"
#include "builtin-elf-voc-entry.hh"
#include "builtin-elf-voc-etype.hh"
#include "builtin-elf-voc-flags.hh"
#include "builtin-elf-voc-label.hh"
#include "builtin-elf-voc-link.hh"
#include "builtin-elf-voc-name.hh"
#include "builtin-elf-voc-osabi.hh"
#include "builtin-elf-voc-section.hh"
#include "builtin-elf-voc-shstr.hh"
#include "builtin-elf-voc-size.hh"
#include "builtin-elf-voc-symbol.hh"
#include "builtin-elf-voc-value.hh"
#include "builtin-elf-voc-version.hh"
#include "builtin-elf-voc-visibility.hh"
#include "builtin-symbol.hh"
#include "elfcst.hh"
#include "known-elf.h"
#include "known-elf-extra.h"

std::unique_ptr <vocabulary>
dwgrep_vocabulary_elf ()
{
  auto ret = std::make_unique <vocabulary> ();
  vocabulary &voc = *ret;

  add_builtin_type_constant <value_symbol> (voc);
  add_builtin_type_constant <value_elf> (voc);
  add_builtin_type_constant <value_elf_section> (voc);
  add_builtin_type_constant <value_strtab_entry> (voc);

  dwgrep_vocabulary_elf_abiversion (voc);
  dwgrep_vocabulary_elf_address (voc);
  dwgrep_vocabulary_elf_alignment (voc);
  dwgrep_vocabulary_elf_binding (voc);
  dwgrep_vocabulary_elf_eclass (voc);
  dwgrep_vocabulary_elf_edata (voc);
  dwgrep_vocabulary_elf_eentry (voc);
  dwgrep_vocabulary_elf_eident (voc);
  dwgrep_vocabulary_elf_elf (voc);
  dwgrep_vocabulary_elf_emachine (voc);
  dwgrep_vocabulary_elf_entry (voc);
  dwgrep_vocabulary_elf_etype (voc);
  dwgrep_vocabulary_elf_flags (voc);
  dwgrep_vocabulary_elf_label (voc);
  dwgrep_vocabulary_elf_link (voc);
  dwgrep_vocabulary_elf_name (voc);
  dwgrep_vocabulary_elf_osabi (voc);
  dwgrep_vocabulary_elf_section (voc);
  dwgrep_vocabulary_elf_shstr (voc);
  dwgrep_vocabulary_elf_size (voc);
  dwgrep_vocabulary_elf_symbol (voc);
  dwgrep_vocabulary_elf_value (voc);
  dwgrep_vocabulary_elf_version (voc);
  dwgrep_vocabulary_elf_visibility (voc);

  return ret;
}
