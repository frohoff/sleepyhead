/* graph spacer Header
 *
 * Copyright (c) 2011-2016 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef GSPACER_H
#define GSPACER_H

#include "gGraphView.h"


/*! \class gSpacer
    \brief A dummy graph spacer layer object
   */
class gSpacer: public Layer
{
  public:
    gSpacer(int space = 20); // orientation?
    virtual void paint(QPainter &painter, gGraph &g, const QRegion &region) {
        Q_UNUSED(painter);
        Q_UNUSED(g);
        Q_UNUSED(region);
    }
    int space() { return m_space; }

  protected:
    int m_space;
};

#endif // GSPACER_H
