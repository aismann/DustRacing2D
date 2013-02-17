// This file is part of Dust Racing 2D.
// Copyright (C) 2013 Jussi Lind <jussi.lind@iki.fi>
//
// Dust Racing 2D is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Dust Racing 2D is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Dust Racing 2D. If not, see <http://www.gnu.org/licenses/>.

#include "music.hpp"

namespace SFX {

bool Music::m_enable = true;
int  Music::m_volume = MIX_MAX_VOLUME;

Music::Music(std::string name, Mix_Music * data)
: m_name(name)
, m_data(data)
{}

int Music::play(int loops, int newVolume)
{
    if (Music::m_enable)
    {
        Mix_VolumeMusic(newVolume);
        return Mix_PlayMusic(m_data, loops);
    }

    return -1;
}

int Music::play(int loops)
{
    if (Music::m_enable)
    {
        Mix_VolumeMusic(Music::m_volume);
        return Mix_PlayMusic(m_data, loops);
    }

    return -1;
}

void Music::stop()
{
    if (Music::m_enable)
    {
        Mix_HaltMusic();
    }
}

void Music::enable(bool flag)
{
    Music::m_enable = flag;
}

void Music::setVolume(int newVolume)
{
    if (Music::m_enable)
    {
        Music::m_volume = newVolume;
        Mix_VolumeMusic(newVolume);
    }
}

int Music::volume()
{
    return Music::m_volume;
}

Music::~Music()
{}

} // namespace SFX
