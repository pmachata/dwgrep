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

#ifndef LIBZWERG_MACH_H_
#define LIBZWERG_MACH_H_

#include <libzwerg.h>

#ifdef __cplusplus
extern "C" {
#endif

  // Objects of type zw_machine represent a machine architecture.
  typedef struct zw_machine zw_machine;

  // Create a new machine architecture descriptor from an ELF EM_*
  // constant.  Returns NULL on error, in which case it sets *OUT_ERR.
  // OUT_ERR shall be non-NULL.
  zw_machine *zw_machine_init (int code, zw_error **out_err);

  // Release any resources associated with MACHINE.
  void zw_machine_destroy (zw_machine *machine);

  // Return an EM_* code of this MACHINE.
  int zw_machine_code (zw_machine const *machine);

#ifdef __cplusplus
}
#endif

#endif
