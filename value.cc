#include <sstream>
#include <cassert>

#include <boost/format.hpp>

#include "value.hh"
#include "dwpp.hh"

dieref::dieref (Dwarf_Off offset)
  : m_offset (offset)
{}

dieref::dieref (Dwarf_Die *die)
  : m_offset (dwarf_dieoffset (die))
{}

Dwarf_Die
dieref::die (std::shared_ptr <Dwarf> &dw) const
{
  return dwpp_offdie (&*dw, m_offset);
}

Dwarf_Off
dieref::offset () const
{
  return m_offset;
}


subgraph::subgraph (std::shared_ptr <Dwarf> dw)
  : m_dw (dw)
{}

subgraph::subgraph (subgraph &&other)
  : m_dw (std::move (other.m_dw))
  , m_dies (std::move (other.m_dies))
{}

void
subgraph::add (dieref const &d)
{
  m_dies.push_back (d);
}

Dwarf_Die
subgraph::focus () const
{
  assert (m_dies.size () > 0);
  return m_dies.back ().die (m_dw);
}

std::string
subgraph::format () const
{
  std::stringstream ss;
  ss << "{" << std::hex;
  for (auto const &dr: m_dies)
    ss << dr.offset () << ", ";
  ss << "}";
  return ss.str ();
}


std::string
ivalue::format () const
{
  return str (boost::format ("%d") % m_val);
}


std::string
fvalue::format () const
{
  return str (boost::format ("%f") % m_val);
}


std::string
svalue::format () const
{
  return m_str;
}


std::string
sequence::format () const
{
  std::stringstream ss;
  ss << "(";
  for (auto const &val: m_seq)
    ss << val->format () << ", ";
  ss << ")";
  return ss.str ();
}
