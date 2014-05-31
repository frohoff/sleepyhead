/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * gBarChart Header
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef GBARCHART_H
#define GBARCHART_H

#include <SleepLib/profiles.h>
#include "gGraphView.h"
#include "gXAxis.h"

/*! \enum GraphType
    \value GT_BAR   Display as a BarGraph
    \value GT_LINE  Display as a line plot
    \value GT_SESSIONS Display type for session times chart
    */
enum GraphType { GT_BAR, GT_LINE, GT_POINTS, GT_SESSIONS };

/*! \class SummaryChart
    \brief The main overall chart type layer used in Overview page
    */
class SummaryChart: public Layer
{
  public:
    //! \brief Constructs a SummaryChart with QString label, of GraphType type
    SummaryChart(QString label, GraphType type = GT_BAR);
    virtual ~SummaryChart();

    //! \brief Renders the graph to the QPainter object
    virtual void paint(QPainter &painter, gGraph &w, const QRegion &region);

    //! \brief Precalculation code prior to drawing. Day object is not needed here, it's just here for Layer compatability.
    virtual void SetDay(Day *day = nullptr);

    //! \brief Returns true if no data was found for this day during SetDay
    virtual bool isEmpty() { return m_empty; }

    //! \brief Adds a layer to the summaryChart (When in Bar mode, it becomes culminative, eg, the AHI chart)
    void addSlice(ChannelID code, QColor color, SummaryType type, EventDataType tval = 0.00f) {
        m_codes.push_back(code);
        m_colors.push_back(color);
        m_type.push_back(type);
        //m_zeros.push_back(ignore_zeros);
        m_typeval.push_back(tval);
    }

    //! \brief Deselect highlighting (the gold bar)
    virtual void deselect() {
        hl_day = -1;
    }

    //! \brief Returns true if currently selected..
    virtual bool isSelected() { return hl_day >= 0; }


    //! \brief Sets the MachineType this SummaryChart is interested in
    void setMachineType(MachineType type) { m_machinetype = type; }

    //! \brief Returns the MachineType this SummaryChart is interested in
    MachineType machineType() { return m_machinetype; }
  protected:
    Qt::Orientation m_orientation;

    QVector<QColor> m_colors;
    QVector<ChannelID> m_codes;
    QVector<bool> m_goodcodes;
    //QVector<bool> m_zeros;
    QVector<SummaryType> m_type;
    QVector<EventDataType> m_typeval;
    QHash<int, QHash<short, EventDataType> > m_values;
    QHash<int, QHash<short, EventDataType> > m_times;
    QHash<int, EventDataType> m_hours;
    QHash<int, Day *> m_days;

    bool m_empty;
    int m_fday;
    QString m_label;

    float barw; // bar width from last draw
    qint64 l_offset; // last offset
    float offset;    // in pixels;
    int l_left, l_top, l_width, l_height;
    int rtop;
    qint64 l_minx, l_maxx;
    int hl_day;
    gGraph *graph;
    GraphType m_graphtype;
    MachineType m_machinetype;
    int tz_offset;
    float tz_hours;

    //! \brief Key was pressed that effects this layer
    virtual bool keyPressEvent(QKeyEvent *event, gGraph *graph);

    //! \brief Mouse moved over this layers area (shows the hover-over tooltips here)
    virtual bool mouseMoveEvent(QMouseEvent *event, gGraph *graph);

    //! \brief Mouse Button was pressed over this area
    virtual bool mousePressEvent(QMouseEvent *event, gGraph *graph);

    //! \brief Mouse Button was released over this area. (jumps to daily view here)
    virtual bool mouseReleaseEvent(QMouseEvent *event, gGraph *graph);
};


#endif // GBARCHART_H
