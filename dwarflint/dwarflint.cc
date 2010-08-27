/* Dwarflint check scheduler.
   Copyright (C) 2008,2009 Red Hat, Inc.
   This file is part of Red Hat elfutils.

   Red Hat elfutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by the
   Free Software Foundation; version 2 of the License.

   Red Hat elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with Red Hat elfutils; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301 USA.

   Red Hat elfutils is an included package of the Open Invention Network.
   An included package of the Open Invention Network is a package for which
   Open Invention Network licensees cross-license their patents.  No patent
   license is granted, either expressly or impliedly, by designation as an
   included package.  Should you wish to participate in the Open Invention
   Network licensing program, please visit www.openinventionnetwork.com
   <http://www.openinventionnetwork.com>.  */

#include "dwarflint.hh"
#include "messages.h"
#include "checks.hh"

#include <fcntl.h>
#include <cstring>
#include <cerrno>
#include <stdexcept>
#include <sstream>

namespace
{
  int
  get_fd (char const *fname)
  {
    /* Open the file.  */
    int fd = open (fname, O_RDONLY);
    if (fd == -1)
      {
	std::stringstream ss;
	ss << "Cannot open input file: " << strerror (errno) << ".";
	throw std::runtime_error (ss.str ());
      }

    return fd;
  }
}

dwarflint::dwarflint (char const *a_fname, check_rules const &rules)
  : _m_fname (a_fname)
  , _m_fd (get_fd (_m_fname))
  , _m_rules (rules)
{
  check_registrar::inst ()->enroll (*this);
}

dwarflint::~dwarflint ()
{
  if (close (_m_fd) < 0)
    // Not that we can do anything about it...
    wr_error () << "Couldn't close the file " << _m_fname << ": "
		<< strerror (errno) << "." << std::endl;
  for (check_map::const_iterator it = _m_checks.begin ();
       it != _m_checks.end (); ++it)
    delete it->second;
}

void
dwarflint::check_registrar::enroll (dwarflint &lint)
{
  for (std::vector <item *>::iterator it = _m_items.begin ();
       it != _m_items.end (); ++it)
    {
      checkstack stack;
      (*it)->run (stack, lint);
    }
}

void
dwarflint::check_registrar::list_checks () const
{
  for (std::vector <item *>::const_iterator it = _m_items.begin ();
       it != _m_items.end (); ++it)
    (*it)->list ();
}

void
dwarflint::list_check (checkdescriptor const &cd)
{
  std::cout << cd.name << std::endl;
}

namespace
{
  bool
  rule_matches (std::string const &name,
		checkdescriptor const &d)
  {
    if (name == "@all")
      return true;
    if (name == "@none")
      return false;
    if (name == d.name)
      return true;
    for (std::vector<std::string>::const_iterator it = d.groups.begin ();
	 it != d.groups.end (); ++it)
      if (name == *it)
	return true;
    return false;
  }
}

bool
check_rules::should_check (checkstack const &stack) const
{
#if 0
  std::cout << "---\nstack" << std::endl;
  for (checkstack::const_iterator jt = stack.begin ();
       jt != stack.end (); ++jt)
    std::cout << (*jt)->name << std::flush << "  ";
  std::cout << std::endl;
#endif

  bool should = false;
  for (const_iterator it = begin (); it != end (); ++it)
    {
      std::string const &rule_name = it->name;
      bool nflag = it->action == check_rule::request;
      if (nflag == should)
	continue;

      for (checkstack::const_iterator jt = stack.begin ();
	   jt != stack.end (); ++jt)
	if (rule_matches (rule_name, **jt))
	  {
	    //std::cout << " rule: " << rule_name << " " << nflag << std::endl;
	    should = nflag;
	    break;
	  }
    }

  return should;
}


void *const dwarflint::marker = (void *)-1;

void *
dwarflint::find_check (void const *key)
{
  check_map::const_iterator it = _m_checks.find (key);

  if (it != _m_checks.end ())
    {
      void *c = it->second;

      // We already tried to do the check, but failed.
      if (c == NULL)
	throw check_base::failed ();
      else
	// Recursive dependency!
	assert (c != marker);

      return c;
    }

  return NULL;
}
