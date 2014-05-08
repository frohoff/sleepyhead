/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * gXAxis Header
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef GXAXIS_H
#define GXAXIS_H

#include <QImage>
#include "Graphs/layer.h"

/*! \class gXAxis
    \brief Draws the XTicker timescales underneath graphs */
class gXAxis: public Layer
{
  public:
    gXAxis(QColor col = Qt::black, bool fadeout = true);
    virtual ~gXAxis();
    virtual void paint(QPainter &painter, gGraph &w, int left, int top, int width, int height);
    static const int Margin = 20; // How much room does this take up. (Bottom margin)
    void SetShowMinorLines(bool b) { m_show_minor_lines = b; }
    void SetShowMajorLines(bool b) { m_show_major_lines = b; }
    bool ShowMinorLines() { return m_show_minor_lines; }
    bool ShowMajorLines() { return m_show_major_lines; }
    void SetShowMinorTicks(bool b) { m_show_minor_ticks = b; }
    void SetShowMajorTicks(bool b) { m_show_major_ticks = b; }
    bool ShowMinorTicks() { return m_show_minor_ticks; }
    bool ShowMajorTicks() { return m_show_major_ticks; }
    void setUtcFix(bool b) { m_utcfix = b; }

  protected:
    //        virtual const wxString & Format(double v) { static wxString t; wxDateTime d; d.Set(v); t=d.Format(wxT("%H:%M")); return t; };
    bool m_show_major_lines;
    bool m_show_minor_lines;
    bool m_show_minor_ticks;
    bool m_show_major_ticks;

    bool m_utcfix;

    QColor m_line_color;
    QColor m_text_color;
    QColor m_major_color;
    QColor m_minor_color;
    bool m_fadeout;
    qint64 tz_offset;
    float tz_hours;

    QImage m_image;
};
#endif // GXAXIS_H
