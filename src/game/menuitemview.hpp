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

#ifndef MENUITEMVIEW_HPP
#define MENUITEMVIEW_HPP

class MenuItem;

//! Base class for menu item views.
class MenuItemView
{
public:

    //! Constructor.
    MenuItemView(MenuItem & owner);

    //! Destructor.
    virtual ~MenuItemView();

    virtual void render(int x, int y);

    virtual void setTextSize(int size);

private:

    MenuItem & m_owner;
    int m_textSize;
};

#endif // MENUITEMVIEW_HPP
