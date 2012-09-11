// This file is part of Dust Racing (DustRAC).
// Copyright (C) 2012 Jussi Lind <jussi.lind@iki.fi>
//
// DustRAC is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// DustRAC is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with DustRAC. If not, see <http://www.gnu.org/licenses/>.

#ifndef SETTINGS_HPP
#define SETTINGS_HPP

class Track;

//! Singleton settings class that wraps the use of QSettings.
class Settings
{
public:

    //! Constructor.
    Settings();

    static Settings & instance();

    void saveLapRecord(Track & track, int msecs);

    int loadLapRecord(Track & track) const;

    void saveBestPos(Track & track, int pos);

    int loadBestPos(Track & track) const;

private:

    static Settings * m_instance;
};

#endif // SETTINGS_HPP
