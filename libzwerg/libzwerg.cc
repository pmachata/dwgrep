/*
  Copyright (C) 2014 Red Hat, Inc.

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

#include "libzwerg.h"

#include <string>
#include <iostream>

#include "builtin.hh"
#include "builtin-dw.hh"

extern "C"
struct zw_error_s
{
  std::string m_message;
};

namespace
{
  zw_error
  zw_error_new (char const *message)
  {
    return new zw_error_s {message};
  }

  template <class U>
  U
  allocate_error (char const *message, U fail_return, zw_error err)
  {
    assert (err != nullptr);
    try
      {
	*err = *zw_error_new (message);
	return fail_return;
      }
    catch (std::exception const &exc)
      {
	std::cerr << "Error: " << message << "\n"
		  << "There was an error while handling that error: "
		  << exc.what () << "\n"
		  << "Aborting.\n";
	std::abort ();
      }
    catch (...)
      {
	std::cerr << "Error: " << message << "\n"
		  << "There was an unknown error while handling that error.\n"
		  << "Aborting.\n";
	std::abort ();
      }
  }

  template <class T>
  auto
  capture_errors (T &&callback, decltype (callback ()) fail_return,
		  zw_error err) -> auto
  {
    try
      {
	return callback ();
      }
    catch (std::exception const &exc)
      {
	return allocate_error (exc.what (), fail_return, err);
      }
    catch (...)
      {
	return allocate_error ("unknown error", fail_return, err);
      }
  }
}

extern "C"
struct zw_vocabulary_s
{
  std::unique_ptr <vocabulary> m_voc;
};

extern "C" void
zw_error_destroy (zw_error err)
{
  delete err;
}

extern "C" char const *
zw_error_message (zw_error err)
{
  return err->m_message.c_str ();
}

extern "C" zw_vocabulary
zw_vocabulary_init (zw_error out_err)
{
  return capture_errors ([&] () { return new zw_vocabulary_s {}; },
			 nullptr, out_err);
}

extern "C" void
zw_vocabulary_destroy (zw_vocabulary voc)
{
  delete voc;
}

extern "C" c_zw_vocabulary
zw_vocabulary_core (void)
{
  static zw_vocabulary_s v {dwgrep_vocabulary_core ()};
  return &v;
}

extern "C" c_zw_vocabulary
zw_vocabulary_dwarf (void)
{
  static zw_vocabulary_s v {dwgrep_vocabulary_dw ()};
  return &v;
}
