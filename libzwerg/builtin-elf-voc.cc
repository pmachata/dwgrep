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

#include "builtin-elf.hh"
#include "builtin-elf-voc.hh"
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

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_name_symbol> ();
    t->add_op_overload <op_name_elf> ();
    t->add_op_overload <op_name_elfscn> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("name", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_address_symbol> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("value", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_address_symbol> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("address", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_label_symbol> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("label", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_symbol_dwarf> ();
    t->add_op_overload <op_symbol_elf> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("symbol", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_binding_symbol> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("binding", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_visibility_symbol> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("visibility", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_size_symbol> ();
    t->add_op_overload <op_size_elfscn> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("size", t));
  }

#define ELF_ONE_KNOWN_STT(NAME, CODE)					\
  add_builtin_constant (voc, constant (CODE, &elfsym_stt_dom (machine)), #CODE);

  {
    constexpr int machine = EM_NONE;
    ELF_ALL_KNOWN_STT
  }

#define ELF_ONE_KNOWN_STT_ARCH(ARCH)		\
    {						\
      constexpr int machine = EM_##ARCH;	\
      ELF_ALL_KNOWN_STT_##ARCH			\
    }
  ELF_ALL_KNOWN_STT_ARCHES

#undef ELF_ONE_KNOWN_STT_ARCH
#undef ELF_ONE_KNOWN_STT


#define ELF_ONE_KNOWN_STB(NAME, CODE)					\
  add_builtin_constant (voc, constant (CODE, &elfsym_stb_dom (machine)), #CODE);

  {
    constexpr int machine = EM_NONE;
    ELF_ALL_KNOWN_STB
  }

#define ELF_ONE_KNOWN_STB_ARCH(ARCH)		\
    {						\
      constexpr int machine = EM_##ARCH;	\
      ELF_ALL_KNOWN_STB_##ARCH			\
    }
  ELF_ALL_KNOWN_STB_ARCHES

#undef ELF_ONE_KNOWN_STB_ARCH
#undef ELF_ONE_KNOWN_STB


#define ELF_ONE_KNOWN_STV(NAME, CODE)					\
  add_builtin_constant (voc, constant (CODE, &elfsym_stv_dom ()), #CODE);

    ELF_ALL_KNOWN_STV

#undef ELF_ONE_KNOWN_STV

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_elf_dwarf> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("elf", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_eclass_elf <value_dwarf>> ();
    t->add_op_overload <op_eclass_elf <value_elf>> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("eclass", t));
  }

#define ADD_ELF_CONSTANT(CODE, NAME, DOM, PRED)				\
    {									\
      add_builtin_constant (voc, constant (CODE, DOM), NAME);		\
									\
      auto t = std::make_shared <overload_tab> ();			\
									\
      t->add_pred_overload <PRED <value_dwarf>> (CODE);			\
      t->add_pred_overload <PRED <value_elf>> (CODE);			\
									\
      voc.add (std::make_shared <overloaded_pred_builtin>		\
	       ("?" NAME, t, true));					\
      voc.add (std::make_shared <overloaded_pred_builtin>		\
	       ("!" NAME, t, false));					\
    }

#define ELF_ONE_KNOWN_ELFCLASS(CODE)		   \
  ADD_ELF_CONSTANT(CODE, #CODE, &elf_class_dom (), pred_eclass_elf);

  ELF_ALL_KNOWN_ELFCLASS
#undef ELF_ONE_KNOWN_ELFCLASS

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_edata_elf <value_dwarf>> ();
    t->add_op_overload <op_edata_elf <value_elf>> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("edata", t));
  }

#define ELF_ONE_KNOWN_ELFDATA(CODE)		   \
  ADD_ELF_CONSTANT(CODE, #CODE, &elf_data_dom (), pred_edata_elf);

  ELF_ALL_KNOWN_ELFDATA
#undef ELF_ONE_KNOWN_ELFDATA

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_etype_elf <value_elf>> ();
    t->add_op_overload <op_etype_elf <value_dwarf>> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("etype", t));
  }

#define ELF_ONE_KNOWN_ET(NAME, CODE)			\
  ADD_ELF_CONSTANT(CODE, #CODE, &elf_et_dom (), pred_etype_elf);

ELF_ALL_KNOWN_ET
#undef ELF_ONE_KNOWN_ET

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_emachine_elf <value_elf>> ();
    t->add_op_overload <op_emachine_elf <value_dwarf>> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("emachine", t));
  }

#define ELF_ONE_KNOWN_EM(NAME, CODE)			\
  ADD_ELF_CONSTANT(CODE, #CODE, &elf_em_dom (), pred_emachine_elf);

ELF_ALL_KNOWN_EM
#undef ELF_ONE_KNOWN_EM

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_version_elf <value_elf>> ();
    t->add_op_overload <op_version_elf <value_dwarf>> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("version", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_eversion_elf <value_elf>> ();
    t->add_op_overload <op_eversion_elf <value_dwarf>> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("eversion", t));
  }

  // N.B.: There are no ?EV_ and !EV_ words. It's not clear whether the version
  // should be tested against EI_VERSION or e_version. Not that it matters much,
  // since both are just "1", but since it doesn't, it's no problem to omit
  // these.
#define ELF_ONE_KNOWN_EV(CODE)						\
  add_builtin_constant (voc, constant (CODE, &elf_ev_dom ()), #CODE);

ELF_ALL_KNOWN_EV
#undef ELF_ONE_KNOWN_EV


  // N.B.: There are no ?EF_ and !EF_ words. Flags need to be tested explicitly,
  // because the field often includes various bitfields and it's not a priori
  // clear how the flag should be tested.
#define ELF_ONE_KNOWN_EF(NAME, CODE)					\
  add_builtin_constant (voc, constant (CODE, &elf_ef_dom (machine)), #CODE);

#define ELF_ONE_KNOWN_EF_ARCH(ARCH)		\
    {						\
      constexpr int machine = EM_##ARCH;	\
      ELF_ALL_KNOWN_EF_##ARCH			\
    }
  ELF_ALL_KNOWN_EF_ARCHES

#undef ELF_ONE_KNOWN_EF_ARCH
#undef ELF_ONE_KNOWN_EF

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_eflags_elf <value_elf>> ();
    t->add_op_overload <op_eflags_elf <value_dwarf>> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("eflags", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_osabi_elf <value_elf>> ();
    t->add_op_overload <op_osabi_elf <value_dwarf>> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("osabi", t));
  }

#define ELF_ONE_KNOWN_ELFOSABI(NAME, CODE)				\
  ADD_ELF_CONSTANT(CODE, #CODE, &elf_osabi_dom (), pred_osabi_elf);

ELF_ALL_KNOWN_ELFOSABI
#undef ELF_ONE_KNOWN_ELFOSABI

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_eentry_elf <value_elf>> ();
    t->add_op_overload <op_eentry_elf <value_dwarf>> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("eentry", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_abiversion_elf <value_elf>> ();
    t->add_op_overload <op_abiversion_elf <value_dwarf>> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("abiversion", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_eident_elf <value_elf>> ();
    t->add_op_overload <op_eident_elf <value_dwarf>> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("eident", t));
  }

#define ELF_ONE_KNOWN_EI(NAME, CODE)				\
  add_builtin_constant (voc, constant (CODE, &dec_constant_dom), #CODE);

ELF_ALL_KNOWN_EI
#undef ELF_ONE_KNOWN_EI

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_shstr_elf <value_elf>> ();
    t->add_op_overload <op_shstr_elf <value_dwarf>> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("shstr", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_section_elf <value_elf>> ();
    t->add_op_overload <op_section_elf <value_dwarf>> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("section", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_entry_elfscn> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("entry", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_link_elfscn> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("link", t));
  }

#define ELF_ONE_KNOWN_SHT(NAME, CODE)					\
  add_builtin_constant (voc, constant (CODE, &elf_sht_dom (machine)), #CODE);

  {
    constexpr int machine = EM_NONE;
    ELF_ALL_KNOWN_SHT
  }

#define ELF_ONE_KNOWN_SHT_ARCH(ARCH)		\
    {						\
      constexpr int machine = EM_##ARCH;	\
      ELF_ALL_KNOWN_SHT_##ARCH			\
    }
  ELF_ALL_KNOWN_SHT_ARCHES

#undef ELF_ONE_KNOWN_SHT_ARCH
#undef ELF_ONE_KNOWN_SHT

  return ret;
}
