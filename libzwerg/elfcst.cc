/*
   Copyright (C) 2018 Petr Machata

   This file is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <elf.h>
#include "constant.hh"
#include "dwcst.hh"

static const char *
elf_class_string (int cls, brevity brv)
{
  switch (cls)
    {
    case ELFCLASSNONE:
      return abbreviate ("ELFCLASSNONE", 8, brv);
    case ELFCLASS32:
      return abbreviate ("ELFCLASS32", 8, brv);
    case ELFCLASS64:
      return abbreviate ("ELFCLASS64", 8, brv);
    default:
      return nullptr;
    }
}

zw_cdom const &
elf_class_dom ()
{
  static dw_simple_dom dom {"ELFCLASS", elf_class_string, 0, 0, false};
  return dom;
}
