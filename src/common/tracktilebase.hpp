// This file is part of Dust Racing 2D.
// Copyright (C) 2011 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef TRACKTILEBASE_HPP
#define TRACKTILEBASE_HPP

#include <QPoint>
#include <QPointF>
#include <QString>

class TrackDataBase;

//! Base class for track tiles used in the editor and in the game.
class TrackTileBase
{
public:

    //! Hints for computer players.
    enum ComputerHint
    {
        CH_NONE = 0,
        CH_BRAKE_HARD,
        CH_BRAKE
    };

    //! Constructor.
    //! \param trackData Reference to the "parent" TrackData.
    //! \param location Location (coordinates) in the track scene.
    //! \param matrixLocation Location in the tile matrix.
    //! \param type Type of the tile. See setType().
    TrackTileBase(TrackDataBase & trackData, QPointF location, QPoint matrixLocation,
        const QString & type = "clear");

    //! Destructor.
    virtual ~TrackTileBase();

    //! Return reference to the "parent" track data.
    TrackDataBase & trackData() const;

    //! Get location in world / scene.
    virtual QPointF location() const;

    //! Get location in the tile matrix.
    virtual QPoint matrixLocation() const;

    //! Set type: "corner", "straight", "grass", "finish", "clear"..
    void setTileType(const QString & type);

    //! Get type
    const QString & tileType() const;

    //! Set computer hint
    void setComputerHint(ComputerHint hint);

    //! Get computer hint
    ComputerHint computerHint() const;

private:

    //! Type string.
    QString m_tileType;

    //! Location in the world / scene.
    QPointF m_location;

    //! Location in the tile matrix.
    QPoint m_matrixLocation;

    //! Pointer to the "parent" track data.
    TrackDataBase & m_trackData;

    //! Computer hint
    ComputerHint m_computerHint;
};

#endif // TRACKTILEBASE_HPP
