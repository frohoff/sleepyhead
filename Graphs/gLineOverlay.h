/*
 gLineOverlayBar Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#ifndef GLINEOVERLAY_H
#define GLINEOVERLAY_H

#include "gGraphView.h"

/*! \class gLineOverlayBar
    \brief Shows a flag line, a dot, or a span over the top of a 2D line chart.
    */
class gLineOverlayBar:public Layer
{
    public:
        //! \brief Constructs a gLineOverlayBar object of type code, setting the flag/span color, the label to show when zoomed
        //! in, and the display method requested, of either FT_Bar, FT_Span, or FT_Dot
        gLineOverlayBar(ChannelID code,QColor col,QString _label="",FlagType _flt=FT_Bar);
        virtual ~gLineOverlayBar();

        //! \brief The drawing code that fills the OpenGL vertex GLBuffers
        virtual void paint(gGraph & w,int left, int top, int width, int height);

        virtual EventDataType Miny() { return 0; }
        virtual EventDataType Maxy() { return 0; }

        //! \brief Returns true if no Channel data available for this code
        virtual bool isEmpty() { return true; }

        //! \brief returns a count of all flags drawn during render. (for gLineOverlaySummary)
        int count() { return m_count; }
    protected:
        QColor m_flag_color;
        QString m_label;
        FlagType m_flt;
        int m_count;

        GLShortBuffer *points,*quads, *lines;
};

/*! \class gLineOverlaySummary
    \brief A container class to hold a group of gLineOverlayBar's, and shows the "Section AHI" over the Flow Rate waveform.
    */
class gLineOverlaySummary:public Layer
{
    public:
        gLineOverlaySummary(QString text, int x, int y);
        virtual ~gLineOverlaySummary();

        virtual void paint(gGraph & w,int left, int top, int width, int height);
        virtual EventDataType Miny() { return 0; }
        virtual EventDataType Maxy() { return 0; }

        //! \brief Returns true if no Channel data available for this code
        virtual bool isEmpty() { return true; }

        //! \brief Adds a gLineOverlayBar to this list
        gLineOverlayBar *add(gLineOverlayBar *bar) { m_overlays.push_back(bar); return bar; }
    protected:
        QVector<gLineOverlayBar *> m_overlays;
        QString m_text;
        int m_x,m_y;
  };


#endif // GLINEOVERLAY_H
