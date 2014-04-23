/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * SleepLib Day Class Implementation
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include <QMultiMap>

#include <algorithm>
#include <cmath>
#include <limits>

#include "day.h"
#include "profiles.h"

Day::Day(Machine *m)
    : machine(m)
{
    d_firstsession = true;
}
Day::~Day()
{
    for (auto s = sessions.begin(); s != sessions.end(); ++s) {
        delete(*s);
    }
}
MachineType Day::machine_type() const
{
    return machine->GetType();
}
Session *Day::find(SessionID sessid)
{
    auto end=sessions.end();
    for (auto s = sessions.begin(); s != end; ++s) {
        if ((*s)->session() == sessid) {
            return (*s);
        }
    }

    return NULL;
}

void Day::AddSession(Session *s)
{
    if (!s) {
        qWarning("Day::AddSession called with NULL session object");
        return;
    }

    sessions.push_back(s);
}

EventDataType Day::settings_sum(ChannelID code)
{
    EventDataType val = 0;

    auto end = sessions.end();
    for (auto it = sessions.begin(); it != end; ++it) {
        Session &sess = *(*it);

        if (sess.enabled()) {
            auto set = sess.settings.find(code);

            if (set != sess.settings.end()) {
                val += set.value().toDouble();
            }
        }
    }

    return val;
}

EventDataType Day::settings_max(ChannelID code)
{
    EventDataType min = std::numeric_limits<EventDataType>::min();
    EventDataType max = min;
    EventDataType value;

    auto end = sessions.end();
    for(auto it = sessions.begin(); it < end; ++it) {
        Session &sess = *(*it);
        if (sess.enabled()) {
            value = sess.settings.value(code, min).toDouble();
            if (value > max) {
                max = value;
            }
        }
    }

    return max;
}

EventDataType Day::settings_min(ChannelID code)
{
    EventDataType max = std::numeric_limits<EventDataType>::max();
    EventDataType min = max;
    EventDataType value;

    auto end=sessions.end();

    for(auto it = sessions.begin(); it < end; ++it) {
        Session &sess = *(*it);
        if (sess.enabled()) {
            value = sess.settings.value(code, max).toDouble();
            if (value < min) {
                min = value;
            }
        }
    }

    return min;
}

EventDataType Day::settings_avg(ChannelID code)
{
    EventDataType val = 0;
    int cnt = 0;

    auto end = sessions.end();
    for (auto it = sessions.begin(); it != end; it++) {
        Session &sess = *(*it);
        if (sess.enabled()) {
            auto set = sess.settings.find(code);

            if (set != sess.settings.end()) {
                val += set.value().toDouble();
                cnt++;
            }
        }
    }

    val = (cnt > 0) ? val /= EventDataType(cnt) : val;

    return val;
}

EventDataType Day::settings_wavg(ChannelID code)
{
    double s0 = 0, s1 = 0, s2 = 0, tmp;

    auto end = sessions.end();
    for (auto it = sessions.begin(); it != end; it++) {
        Session &sess = *(*it);

        if (sess.enabled()) {
            auto set = sess.settings.find(code);

            if (set != sess.settings.end()) {
                s0 = sess.hours();
                tmp = set.value().toDouble();
                s1 += tmp * s0;
                s2 += s0;
            }
        }
    }

    if (s2 == 0) { return 0; }

    tmp = (s1 / s2);
    return tmp;
}


EventDataType Day::percentile(ChannelID code, EventDataType percentile)
{
    // Cache this calculation?
    //    QHash<ChannelID, QHash<EventDataType, EventDataType> >::iterator pi;
    //    pi=perc_cache.find(code);
    //    if (pi!=perc_cache.end()) {
    //        QHash<EventDataType, EventDataType> & hsh=pi.value();
    //        QHash<EventDataType, EventDataType>::iterator hi=hsh.find(
    //        if (hi!=pi.value().end()) {
    //            return hi.value();
    //        }
    //    }

    QHash<EventStoreType, qint64> wmap; // weight map

    qint64 SN = 0;

    EventDataType lastgain = 0, gain = 0;
    // First Calculate count of all events
    bool timeweight;

    auto sess_end = sessions.end();
    for (auto sess_it = sessions.begin(); sess_it != sess_end; sess_it++) {
        Session &sess = *(*sess_it);
        if (!sess.enabled()) { continue; }

        auto ei = sess.m_valuesummary.find(code);

        if (ei == sess.m_valuesummary.end()) { continue; }

        auto tei = sess.m_timesummary.find(code);
        timeweight = (tei != sess.m_timesummary.end());
        gain = sess.m_gain[code];

        // Here's assuming gains don't change accross a days sessions
        // Can't assume this in any multi day calculations..
        if (lastgain > 0) {
            if (gain != lastgain) {
                qDebug() << "Gains differ across sessions: " << gain << lastgain;
            }
        }

        lastgain = gain;

        int value;
        qint64 weight;

        //qint64 tval;
        if (timeweight) {
            auto teival_end = tei.value().end();
            for (auto it = tei.value().begin(); it != teival_end; ++it) {
                value = it.key();
                weight = it.value();
                SN += weight;
                wmap[value] += weight;
            }
        } else {
            auto eival_end = ei.value().end();

            for (auto it = ei.value().begin(); it != eival_end; ++it) {
                value = it.key();
                weight = it.value();

                SN += weight;

                wmap[value] += weight;
            }
        }
    }

    QVector<ValueCount> valcnt;

    // Build sorted list of value/counts
    ValueCount vc;

    auto wmap_end = wmap.end();
    for (auto it = wmap.begin(); it != wmap_end; it++) {
        vc.value = EventDataType(it.key()) * gain;
        vc.count = it.value();
        vc.p = 0;
        valcnt.push_back(vc);
    }

    // sort by weight, then value
    qSort(valcnt);

    //double SN=100.0/double(N); // 100% / overall sum
    double p = 100.0 * percentile;

    double nth = double(SN) * percentile; // index of the position in the unweighted set would be
    double nthi = floor(nth);

    qint64 sum1 = 0, sum2 = 0;
    qint64 w1, w2 = 0;
    double v1 = 0, v2;

    int N = valcnt.size();
    int k = 0;

    for (k = 0; k < N; k++) {
        v1 = valcnt[k].value;
        w1 = valcnt[k].count;
        sum1 += w1;

        if (sum1 > nthi) {
            return v1;
        }

        if (sum1 == nthi) {
            break; // boundary condition
        }
    }

    if (k >= N) {
        return v1;
    }

    v2 = valcnt[k + 1].value;
    w2 = valcnt[k + 1].count;
    sum2 = sum1 + w2;
    // value lies between v1 and v2

    double px = 100.0 / double(SN); // Percentile represented by one full value

    // calculate percentile ranks
    double p1 = px * (double(sum1) - (double(w1) / 2.0));
    double p2 = px * (double(sum2) - (double(w2) / 2.0));

    // calculate linear interpolation
    double v = v1 + ((p - p1) / (p2 - p1)) * (v2 - v1);

    return v;

    //                p1.....p.............p2
    //                37     55            70
}

EventDataType Day::p90(ChannelID code)
{
    return percentile(code, 0.90);
}

EventDataType Day::avg(ChannelID code)
{
    double val = 0;
    // Cache this?
    int cnt = 0;

    auto end = sessions.end();
    for (auto it = sessions.begin(); it != end; ++it) {
        Session &sess = *(*it);

        if (sess.enabled() && sess.m_avg.contains(code)) {
            val += sess.avg(code);
            cnt++;  // hmm.. averaging averages doesn't feel right..
        }
    }

    if (cnt == 0) { return 0; }

    return EventDataType(val / float(cnt));
}

EventDataType Day::sum(ChannelID code)
{
    // Cache this?
    EventDataType val = 0;

    auto end = sessions.end();
    for (auto it = sessions.begin(); it != end; ++it) {
        Session &sess = *(*it);

        if (sess.enabled() && sess.m_sum.contains(code)) {
            val += sess.sum(code);
        }
    }

    return val;
}

EventDataType Day::wavg(ChannelID code)
{
    double s0 = 0, s1 = 0, s2 = 0;
    qint64 d;

    auto end = sessions.end();

    for (auto it = sessions.begin(); it != end; ++it) {
        Session &sess = *(*it);

        if (sess.enabled() && sess.m_wavg.contains(code)) {
            d = sess.length(); //.last(code)-sess.first(code);
            s0 = double(d) / 3600000.0;

            if (s0 > 0) {
                s1 += sess.wavg(code) * s0;
                s2 += s0;
            }
        }
    }

    if (s2 == 0) {
        return 0;
    }

    return (s1 / s2);
}

// Total session time in milliseconds
qint64 Day::total_time()
{
    qint64 d_totaltime = 0;
    QMultiMap<qint64, bool> range;
    //range.reserve(size()*2);

    // Remember sessions may overlap..

    auto end = sessions.end();
    for (auto it = sessions.begin(); it != end; ++it) {
        Session &sess = *(*it);

        if (sess.enabled()) {
            range.insert(sess.first(), 0);
            range.insert(sess.last(), 1);
            d_totaltime += sess.length();
        }
    }

    qint64 ti = 0;
    bool b;
    int nest = 0;
    qint64 total = 0;

    // This is my implementation of a typical "brace counting" algorithm mentioned here:
    // http://stackoverflow.com/questions/7468948/problem-calculating-overlapping-date-ranges

    auto rend = range.end();
    for (auto rit = range.begin(); rit != rend; ++rit) {
        b = rit.value();

        if (!b) {
            if (!ti) {
                ti = rit.key();
            }

            nest++;
        } else {
            if (--nest <= 0) {
                total += rit.key() - ti;
                ti = 0;
            }
        }
    }

    if (total != d_totaltime) {
        qDebug() << "Sessions Times overlaps!" << total << d_totaltime;
    }

    return total; //d_totaltime;
}

bool Day::hasEnabledSessions()
{
    auto end = sessions.end();

    for (auto it = sessions.begin(); it != end; ++it) {
        if ((*it)->enabled()) {
            return true;
        }
    }

    return false;
}

/*EventDataType Day::percentile(ChannelID code,double percent)
{
    double val=0;
    int cnt=0;

    for (QList<Session *>::iterator s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        if (sess.eventlist.find(code)!=sess.eventlist.end()) {
            val+=sess.percentile(code,percent);
            cnt++;
        }
    }
    if (cnt==0) return 0;
    return EventDataType(val/cnt);

}*/

qint64 Day::first(ChannelID code)
{
    qint64 date = 0;
    qint64 tmp;

    auto end = sessions.end();
    for (auto it = sessions.begin(); it != end; ++it) {
        Session & sess=*(*it);

        if (sess.enabled()) {
            tmp = sess.first(code);

            if (!tmp) { continue; }

            if (!date) {
                date = tmp;
            } else {
                if (tmp < date) { date = tmp; }
            }
        }
    }

    return date;
}

qint64 Day::last(ChannelID code)
{
    qint64 date = 0;
    qint64 tmp;

    auto end = sessions.end();
    for (auto it = sessions.begin(); it != end; it++) {
        Session & sess = *(*it);

        if (sess.enabled()) {
            tmp = sess.last(code);

            if (!tmp) { continue; }

            if (!date) {
                date = tmp;
            } else {
                if (tmp > date) { date = tmp; }
            }
        }
    }

    return date;
}

EventDataType Day::Min(ChannelID code)
{
    EventDataType min = 0;
    EventDataType tmp;
    bool first = true;

    auto end = sessions.end();
    for (auto it = sessions.begin(); it != end; it++) {
        Session & sess = *(*it);

        if (sess.enabled() && sess.m_min.contains(code)) {

            tmp = sess.Min(code);

            if (first) {
                min = tmp;
                first = false;
            } else {
                if (tmp < min) { min = tmp; }
            }
        }
    }

    return min;
}

EventDataType Day::physMin(ChannelID code)
{
    EventDataType min = 0;
    EventDataType tmp;
    bool first = true;

    auto end = sessions.end();
    for (auto it = sessions.begin(); it != end; ++it) {
        Session & sess = *(*it);

        if (sess.enabled() && sess.m_min.contains(code)) {

            tmp = sess.physMin(code);

            if (first) {
                min = tmp;
                first = false;
            } else {
                if (tmp < min) { min = tmp; }
            }
        }
    }

    return min;
}

bool Day::hasData(ChannelID code, SummaryType type)
{
    bool has = false;

    auto end = sessions.end();

    for (auto it = sessions.begin(); it != end; ++it) {
        Session & sess = *(*it);

        if (sess.enabled()) {
            switch (type) {
            //        case ST_90P:
            //            has=sess->m_90p.contains(code);
            //            break;
            case ST_PERC:
                has = sess.m_valuesummary.contains(code);
                break;

            case ST_MIN:
                has = sess.m_min.contains(code);
                break;

            case ST_MAX:
                has = sess.m_max.contains(code);
                break;

            case ST_CNT:
                has = sess.m_cnt.contains(code);
                break;

            case ST_AVG:
                has = sess.m_avg.contains(code);
                break;

            case ST_WAVG:
                has = sess.m_wavg.contains(code);
                break;

            case ST_CPH:
                has = sess.m_cph.contains(code);
                break;

            case ST_SPH:
                has = sess.m_sph.contains(code);
                break;

            case ST_FIRST:
                has = sess.m_firstchan.contains(code);
                break;

            case ST_LAST:
                has = sess.m_lastchan.contains(code);
                break;

            case ST_SUM:
                has = sess.m_sum.contains(code);
                break;

            default:
                break;
            }

            if (has) { break; }
        }
    }

    return has;
}

EventDataType Day::Max(ChannelID code)
{
    EventDataType max = 0;
    EventDataType tmp;
    bool first = true;

    auto end = sessions.end();
    for (auto it = sessions.begin(); it != end; ++it) {
        Session & sess = *(*it);

        if (sess.enabled() && sess.m_max.contains(code)) {

            tmp = sess.Max(code);

            if (first) {
                max = tmp;
                first = false;
            } else {
                if (tmp > max) { max = tmp; }
            }
        }
    }

    return max;
}

EventDataType Day::physMax(ChannelID code)
{
    EventDataType max = 0;
    EventDataType tmp;
    bool first = true;

    auto end = sessions.end();
    for (auto it = sessions.begin(); it != end; ++it) {
        Session & sess = *(*it);

        if (sess.enabled() && sess.m_max.contains(code)) {
            tmp = sess.physMax(code);

            if (first) {
                max = tmp;
                first = false;
            } else {
                if (tmp > max) { max = tmp; }
            }
        }
    }

    return max;
}
EventDataType Day::cph(ChannelID code)
{
    double sum = 0;

    //EventDataType h=0;

    auto end = sessions.end();
    for (auto it = sessions.begin(); it != end; ++it) {
        Session & sess = *(*it);

        if (sess.enabled() && sess.m_cnt.contains(code)) {
            sum += sess.count(code);
        }
    }

    sum /= hours();
    return sum;
}

EventDataType Day::sph(ChannelID code)
{
    EventDataType sum = 0;
    EventDataType h = 0;

    auto end = sessions.end();
    for (auto it = sessions.begin(); it != end; ++it) {
        Session & sess = *(*it);

        if (sess.enabled() && sess.m_sum.contains(code)) {
            sum += sess.sum(code) / 3600.0; //*sessions[i]->hours();
            //h+=sessions[i]->hours();
        }
    }

    h = hours();
    sum = (100.0 / h) * sum;
    return sum;
}

int Day::count(ChannelID code)
{
    int sum = 0;

    auto end = sessions.end();
    for (auto it = sessions.begin(); it != end; ++it) {
        Session & sess = *(*it);

        if (sess.enabled()) {
            sum += sess.count(code);
        }
    }

    return sum;
}

bool Day::settingExists(ChannelID id)
{
    auto end = sessions.end();
    for (auto it = sessions.begin(); it != end; ++it) {
        Session & sess = *(*it);

        if (sess.enabled()) {
            auto set = sess.settings.find(id);

            if (set != sess.settings.end()) {
                return true;
            }
        }
    }

    return false;
}

bool Day::eventsLoaded()
{
    auto end = sessions.end();
    for (auto it = sessions.begin(); it != end; ++it) {
        Session & sess = *(*it);

        if (sess.eventsLoaded()) {
            return true;
        }
    }

    return false;
}

bool Day::channelExists(ChannelID id)
{
    auto end = sessions.end();
    for (auto it = sessions.begin(); it != end; ++it) {
        Session & sess = *(*it);

        if (sess.enabled() && sess.eventlist.contains(id)) {
            return true;
        }
    }

    return false;
}

bool Day::channelHasData(ChannelID id)
{
    auto end = sessions.end();

    for (auto it = sessions.begin(); it != end; ++it) {
        Session & sess = *(*it);

        if (sess.enabled()) {
            if (sess.channelExists(id)) {
                return true;
            }

            if (sess.m_valuesummary.contains(id)) {
                return true;
            }
        }
    }

    return false;
}

void Day::OpenEvents()
{
    Q_FOREACH(Session * session, sessions) {
        session->OpenEvents();
    }
}

void Day::CloseEvents()
{
    Q_FOREACH(Session * session, sessions) {
        session->TrashEvents();
    }
}

qint64 Day::first()
{
    qint64 date = 0;
    qint64 tmp;

    auto end = sessions.end();
    for (auto it = sessions.begin(); it != end; ++it) {
        Session & sess = *(*it);

        if (sess.enabled()) {
            tmp = sess.first();

            if (!tmp) { continue; }

            if (!date) {
                date = tmp;
            } else {
                if (tmp < date) { date = tmp; }
            }
        }
    }

    return date;
}

//! \brief Returns the last session time of this day
qint64 Day::last()
{
    qint64 date = 0;
    qint64 tmp;

    auto end = sessions.end();

    for (auto it = sessions.begin(); it != end; ++it) {
        Session & sess = *(*it);

        if (sess.enabled()) {
            tmp = sess.last();

            if (!tmp) { continue; }

            if (!date) {
                date = tmp;
            } else {
                if (tmp > date) { date = tmp; }
            }
        }
    }

    return date;
}

void Day::removeSession(Session *sess)
{
    if (sessions.removeAll(sess) < 1) {
        // something went wrong
    }
}
