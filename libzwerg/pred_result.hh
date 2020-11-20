/*
   Copyright (C) 2014 Red Hat, Inc.
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

#ifndef _PRED_RESULT_H_
#define _PRED_RESULT_H_

#include <cstdlib>

enum class pred_result
  {
    no = false,
    yes = true,
    fail,
  };

inline pred_result
operator! (pred_result other)
{
  switch (other)
    {
    case pred_result::no:
      return pred_result::yes;
    case pred_result::yes:
      return pred_result::no;
    case pred_result::fail:
      return pred_result::fail;
    }
  abort ();
}

inline pred_result
operator&& (pred_result a, pred_result b)
{
  if (a == pred_result::fail || b == pred_result::fail)
    return pred_result::fail;
  else
    return bool (a) && bool (b) ? pred_result::yes : pred_result::no;
}

inline pred_result
operator|| (pred_result a, pred_result b)
{
  if (a == pred_result::fail || b == pred_result::fail)
    return pred_result::fail;
  else
    return bool (a) || bool (b) ? pred_result::yes : pred_result::no;
}

#endif /* _PRED_RESULT_H_ */
