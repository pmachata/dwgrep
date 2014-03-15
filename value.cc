#include <sstream>
#include <cassert>

#include <boost/format.hpp>

#include "value.hh"
#include "dwpp.hh"

v_die::v_die (Dwarf_Off offset)
  : m_offset (offset)
{}

v_die::v_die (Dwarf_Die *die)
  : m_offset (dwarf_dieoffset (die))
{}

Dwarf_Die
v_die::die (std::shared_ptr <Dwarf> &dw) const
{
  return dwpp_offdie (&*dw, m_offset);
}

Dwarf_Off
v_die::offset () const
{
  return m_offset;
}

std::string
v_die::format () const
{
  std::ostringstream ss;
  ss << "[" << std::hex << m_offset << "]";
  return ss.str ();
}

std::unique_ptr <value>
v_die::clone () const
{
  return std::unique_ptr <value> (new v_die { m_offset });
}


v_at::v_at (Dwarf_Off offset, int atname)
  : m_dieoffset (offset)
  , m_atname (atname)
{}

int
v_at::atname () const
{
  return m_atname;
}

std::string
v_at::format () const
{
  std::ostringstream ss;
  ss << "[at:" << m_atname << "@" << std::hex << m_dieoffset << "]";
  return ss.str ();
}

std::unique_ptr <value>
v_at::clone () const
{
  return std::unique_ptr <value> (new v_at { m_dieoffset, m_atname });
}


std::string
v_int::format () const
{
  return str (boost::format ("%d") % m_val);
}


std::string
v_flt::format () const
{
  return str (boost::format ("%f") % m_val);
}


std::string
v_str::format () const
{
  return m_str;
}


std::string
v_seq::format () const
{
  std::stringstream ss;
  ss << "(";
  for (auto const &val: m_seq)
    ss << val->format () << ", ";
  ss << ")";
  return ss.str ();
}
