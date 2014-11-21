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

#include <tuple>
#include "selector.hh"

struct builtin;

// For a given overload, this returns a pair whose first elements is a
// string representation of its prototype, and the second is a
// docstring associated with this overload.  These pairs are suitable
// for the following deduplication step (see below).
std::pair <std::string, std::string>
format_overload (std::tuple <selector, std::shared_ptr <builtin>> const &ovl);

// Entries are a mapping heading->content.  This finds content
// duplicates and merges the corresponding headers.  It returns a
// mapping list of heads->content.
std::vector <std::pair <std::vector <std::string>, std::string>>
doc_deduplicate (std::vector <std::pair <std::string, std::string>> &entries);

// Formats the map that the above deduplication step returned.
std::string format_entry_map (std::vector <std::pair <std::vector <std::string>,
						      std::string>> entries,
			      char udc);
