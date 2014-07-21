/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * gLineOverlayBar Implementation
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include <math.h>
#include "SleepLib/profiles.h"
#include "gLineOverlay.h"

gLineOverlayBar::gLineOverlayBar(ChannelID code, QColor color, QString label, FlagType flt)
    : Layer(code), m_flag_color(color), m_label(label), m_flt(flt)
{
}
gLineOverlayBar::~gLineOverlayBar()
{
}

QColor brighten(QColor color);

void gLineOverlayBar::paint(QPainter &painter, gGraph &w, const QRegion &region)
{
    int left = region.boundingRect().left();
    int topp = region.boundingRect().top(); // FIXME: Misspelling intentional.
    int width = region.boundingRect().width();
    int height = region.boundingRect().height();

    if (!m_visible) { return; }

    if (!m_day) { return; }

    int start_py = topp;

    double xx = w.max_x - w.min_x;
    double yy = w.max_y - w.min_y;
    double jj = double(width) / double(xx);


    if (xx <= 0) { return; }

    float x1, x2;

    int x, y;

    float bottom = start_py + height - 25 * w.printScaleY(), top = start_py + 25 * w.printScaleY();

    double X;
    double Y;

    QPoint mouse=w.graphView()->currentMousePos();

    m_count = 0;
    m_sum = 0;
    m_flag_color = schema::channel[m_code].defaultColor();


    if (m_flt == FT_Span) {
        m_flag_color.setAlpha(128);
    }

    painter.setPen(m_flag_color);

    EventStoreType raw;

    quint32 *tptr;
    EventStoreType *dptr, *eptr;
    qint64 stime;

    OverlayDisplayType odt = p_profile->appearance->overlayType();
    QHash<ChannelID, QVector<EventList *> >::iterator cei;
    int count;

    qint64 clockdrift = qint64(p_profile->cpap->clockDrift()) * 1000L;
    qint64 drift = 0;
    bool hover = false;

    // For each session, process it's eventlist
    for (QList<Session *>::iterator s = m_day->begin(); s != m_day->end(); s++) {

        if (!(*s)->enabled()) { continue; }

        cei = (*s)->eventlist.find(m_code);

        if (cei == (*s)->eventlist.end()) { continue; }

        QVector<EventList *> &evlist = cei.value();

        if (evlist.size() == 0) { continue; }

        drift = ((*s)->machine()->GetType() == MT_CPAP) ? clockdrift : 0;

        // Could loop through here, but nowhere uses more than one yet..
        for (int k = 0; k < evlist.size(); k++) {
            EventList &el = *(evlist[k]);
            count = el.count();
            stime = el.first() + drift;
            dptr = el.rawData();
            eptr = dptr + count;
            tptr = el.rawTime();

            ////////////////////////////////////////////////////////////////////////////
            // Skip data previous to minx bounds
            ////////////////////////////////////////////////////////////////////////////
            for (; dptr < eptr; dptr++) {
                X = stime + *tptr;

                if (X >= w.min_x) {
                    break;
                }

                tptr++;
            }

            if (m_flt == FT_Span) {
                ////////////////////////////////////////////////////////////////////////////
                // FT_Span
                ////////////////////////////////////////////////////////////////////////////
                for (; dptr < eptr; dptr++) {
                    hover = false;

                    X = stime + *tptr++;
                    raw = *dptr;
                    Y = X - (qint64(raw) * 1000.0L); // duration

                    if (Y > w.max_x) {
                        break;
                    }

                    x1 = jj * double(X - w.min_x) + left;
                    m_count++;
                    m_sum += raw;
                    x2 = jj * double(Y - w.min_x) + left;

                    if (int(x1) == int(x2)) {
                        x2 += 1;
                    }

                    if (x2 < left) {
                        x2 = left;
                    }

                    if (x1 > width + left) {
                        x1 = width + left;
                    }

                    QRect rect(x2, start_py, x1-x2, height);
                    QColor col = m_flag_color;
                    if (rect.contains(mouse)) {
                        hover = true;
                    }

                    painter.fillRect(rect, QBrush(col));
                }
            } else if (m_flt == FT_Dot) {
                ////////////////////////////////////////////////////////////////////////////
                // FT_Dot
                ////////////////////////////////////////////////////////////////////////////
                for (; dptr < eptr; dptr++) {
                    hover = false;

                    X = stime + *tptr++; //el.time(i);
                    raw = *dptr; //el.data(i);

                    if (X > w.max_x) {
                        break;
                    }

                    x1 = jj * double(X - w.min_x) + left;
                    m_count++;
                    m_sum += raw;

                    if ((odt == ODT_Bars) || (xx < 3600000)) {
                        // show the fat dots in the middle
                        painter.setPen(QPen(m_flag_color,4));

                        painter.drawPoint(x1, double(height) / double(yy)*double(-20 - w.min_y) + topp);
                        painter.setPen(QPen(m_flag_color,1));

                    } else {
                        // thin lines down the bottom
                        painter.drawLine(x1, start_py + 1, x1, start_py + 1 + 12);
                    }
                }
            } else if (m_flt == FT_Bar) {
                ////////////////////////////////////////////////////////////////////////////
                // FT_Bar
                ////////////////////////////////////////////////////////////////////////////
                for (; dptr < eptr; dptr++) {
                    hover = false;
                    X = stime + *tptr++;
                    raw = *dptr;

                    if (X > w.max_x) {
                        break;
                    }

                    x1 = jj * double(X - w.min_x) + left;
                    m_count++;
                    m_sum += raw;
                    int z = start_py + height;


                    if ((odt == ODT_Bars) || (xx < 3600000)) {
                        z = top;
                        double d1 = jj * double(raw) * 1000.0;

                        QRect rect(x1-d1-2, top, d1+2, height);
                        QColor col = m_flag_color;

                        painter.setPen(QPen(col,4));
                        painter.drawPoint(x1, top);

                        if (rect.contains(mouse)) {
                            QColor col2(230,230,230,128);
                            QRect rect(x1-d1, start_py+2, d1, height-2);
                            if (rect.x() < left) {
                                rect.setX(left);
                            }

                            painter.fillRect(rect, QBrush(col2));
                            painter.setPen(col);
                            painter.drawRect(rect);
//                            painter.drawLine(rect.x(), top, rect.x()+d1, top);
//                            painter.drawLine(rect.x(), bottom, rect.x()+d1, bottom);
//                            painter.drawLine(rect.x(), top, rect.x(), bottom);

                           // col = QColor("gold");
                            hover = true;
                        } else {
                            painter.setPen(QPen(col,1));
                            painter.drawLine(x1, top, x1, bottom);
                        }


                    } else {
                        painter.drawLine(x1, z, x1, z - 12);
                    }

                    QColor txcol = hover ? Qt::red: Qt::black;
                    if (xx < 300000) {
                        QString lab = QString("%1 (%2)").arg(schema::channel[m_code].fullname()).arg(raw);
                        GetTextExtent(lab, x, y);
                        w.renderText(lab, x1 - (x / 2)+2, top - y + (3 * w.printScaleY()),0,txcol);
                    } else if (xx < (3600000)) {
                        if (!hover) {
                            GetTextExtent(m_label, x, y);
                            w.renderText(m_label, x1 - (x / 2)+2, top - y + (3 * w.printScaleY()),0,txcol);
                        } else {
                            QString lab = QString("%1 (%2)").arg(m_label).arg(raw);
                            GetTextExtent(lab, x, y);
                            w.renderText(lab, x1 - (x / 2)+2, top - y + (3 * w.printScaleY()),0,txcol);
                        }
                    }
                }
            }

        }

    }
}
bool gLineOverlayBar::mouseMoveEvent(QMouseEvent *event, gGraph *graph)
{
    Q_UNUSED(event)
    Q_UNUSED(graph)
    return true;
}



gLineOverlaySummary::gLineOverlaySummary(QString text, int x, int y)
    : Layer(CPAP_Obstructive), m_text(text), m_x(x), m_y(y) // The Layer code is a dummy here.
{
}

gLineOverlaySummary::~gLineOverlaySummary()
{
}

void gLineOverlaySummary::paint(QPainter &painter, gGraph &w, const QRegion &region)
{
    Q_UNUSED(painter)

    int left = region.boundingRect().left();
    int top = region.boundingRect().top();
    int width = region.boundingRect().width();
    int height = region.boundingRect().height();

    if (!m_visible) { return; }

    if (!m_day) { return; }


    Q_UNUSED(width);
    Q_UNUSED(height);
    float cnt = 0;
    double sum = 0;
    bool isSpan = false;

    for (int i = 0; i < m_overlays.size(); i++) {
        cnt += m_overlays[i]->count();
        sum += m_overlays[i]->sum();

        if (m_overlays[i]->flagtype() == FT_Span) { isSpan = true; }
    }

    double val, first, last;
    double time = 0;

    // Calculate the session time.
    for (QList<Session *>::iterator s = m_day->begin(); s != m_day->end(); s++) {
        if (!(*s)->enabled()) { continue; }

        first = (*s)->first();
        last = (*s)->last();

        if (last < w.min_x) { continue; }

        if (first > w.max_x) { continue; }

        if (first < w.min_x) {
            first = w.min_x;
        }

        if (last > w.max_x) {
            last = w.max_x;
        }

        time += last - first;
    }

    val = 0;

    time /= 1000;
    int h = time / 3600;
    int m = int(time / 60) % 60;
    int s = int(time) % 60;


    time /= 3600;

    //if (time<1) time=1;

    if (time > 0) { val = cnt / time; }

    QString a;

    if ((w.graphView()->selectionInProgress() || w.graphView()->metaSelect()) && (!w.selDurString().isEmpty())) {
        a = QObject::tr("Duration")+": "+w.selDurString();
    } else {
        a = QObject::tr("Events") + ": " + QString::number(cnt) + ", " +
                QObject::tr("Duration") + " " + QString().sprintf("%02i:%02i:%02i", h, m, s) + ", " + m_text + ": " + QString::number(val, 'f', 2);
    }
    if (isSpan) {
        float sph;

        if (!time) { sph = 0; }
        else {
            sph = (100.0 / float(time)) * (sum / 3600.0);

            if (sph > 100) { sph = 100; }
        }

        // eg: %num of time in a span, like Periodic Breathing
        a += " " + QObject::tr("(% %1 in events)").arg(sph, 0, 'f', 2);
    }

    w.renderText(a, left + m_x, top + m_y);
}
