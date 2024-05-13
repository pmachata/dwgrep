/*
  Copyright (C) 2017 Petr Machata
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

#include "libzwerg.h"
#include "libzwerg-dw.h"

#include <string>
#include <iostream>

#include "scon.hh"
#include "tree.hh"
#include "op.hh"

struct vocabulary;

struct zw_error
{
  std::string m_message;
};

struct zw_vocabulary
{
  std::unique_ptr <vocabulary> m_voc;
};

struct zw_query
{
  layout m_l;
  op_origin &m_origin;
  std::shared_ptr <op> m_op;
};

struct zw_result
{
  std::shared_ptr <op> m_op;
  scon m_sc;
  scon_guard m_sg;

  zw_result (layout const &l,
	     op_origin const &origin, std::shared_ptr <op> op, stack::uptr stk)
    : m_op {op}
    , m_sc {l}
    , m_sg {m_sc, *m_op}
  {
    origin.set_next (m_sc, std::move (stk));
  }

  stack::uptr
  next ()
  {
    return m_op->next (m_sc);
  }
};

struct zw_stack
{
  std::vector <std::unique_ptr <zw_value>> m_values;
};


namespace
{
  __attribute__ ((unused)) zw_error *
  zw_error_new (char const *message)
  {
    return new zw_error {message};
  }

  template <class U>
  U
  allocate_error (char const *message, U fail_return, zw_error **out_err)
  {
    assert (out_err != nullptr);
    try
      {
	*out_err = zw_error_new (message);
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
		  zw_error **out_err) -> decltype (callback ())
  {
    try
      {
	return callback ();
      }
    catch (std::exception const &exc)
      {
	return allocate_error (exc.what (), fail_return, out_err);
      }
    catch (...)
      {
	return allocate_error ("unknown error", fail_return, out_err);
      }
  }
}
