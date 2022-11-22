// This file belongs to the "MiniCore" game engine.
// Copyright (C) 2015 Jussi Lind <jussi.lind@iki.fi>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA  02110-1301, USA.
//

#include "mctyperegistry.hh"

MCTypeRegistry::MCTypeRegistry()
  : m_typeIdCount(1)
{
}

unsigned int MCTypeRegistry::registerType(const std::string & typeName)
{
    if (const auto i(m_typeHash.find(typeName)); i == m_typeHash.end())
    {
        m_typeIdCount++;
        m_typeHash[typeName] = m_typeIdCount;
        return m_typeIdCount;
    }
    else
    {
        return i->second;
    }
}

unsigned int MCTypeRegistry::getTypeIdForName(const std::string & typeName)
{
    const auto i(m_typeHash.find(typeName));
    return i == m_typeHash.end() ? 0 : i->second;
}
