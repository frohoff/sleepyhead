/* Statistics Report Generator Implementation
 *
 * Copyright (c) 2011 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include <QApplication>
#include <cmath>

#include "mainwindow.h"
#include "statistics.h"

extern MainWindow *mainwin;

QString formatTime(float time)
{
    int hours = time;
    int seconds = time * 3600.0;
    int minutes = (seconds / 60) % 60;
    seconds %= 60;
    return QString().sprintf("%02i:%02i", hours, minutes); //,seconds);
}


Statistics::Statistics(QObject *parent) :
    QObject(parent)
{
    rows.push_back(StatisticsRow(tr("CPAP Statistics"), SC_HEADING, MT_CPAP));
    rows.push_back(StatisticsRow("",   SC_DAYS, MT_CPAP));
    rows.push_back(StatisticsRow("", SC_COLUMNHEADERS, MT_CPAP));
    rows.push_back(StatisticsRow(tr("CPAP Usage"),  SC_SUBHEADING, MT_CPAP));
    rows.push_back(StatisticsRow(tr("Average Hours per Night"),      SC_HOURS,     MT_CPAP));
    rows.push_back(StatisticsRow(tr("Compliance"),  SC_COMPLIANCE,  MT_CPAP));

    rows.push_back(StatisticsRow(tr("Therapy Efficiacy"),  SC_SUBHEADING, MT_CPAP));
    rows.push_back(StatisticsRow("AHI",        SC_AHI,     MT_CPAP));
    rows.push_back(StatisticsRow("Obstructive",   SC_CPH,     MT_CPAP));
    rows.push_back(StatisticsRow("Hypopnea",   SC_CPH,     MT_CPAP));
    rows.push_back(StatisticsRow("ClearAirway",   SC_CPH,     MT_CPAP));
    rows.push_back(StatisticsRow("FlowLimit",  SC_CPH,     MT_CPAP));
    rows.push_back(StatisticsRow("RERA",       SC_CPH,     MT_CPAP));
    rows.push_back(StatisticsRow("SensAwake",       SC_CPH,     MT_CPAP));
    rows.push_back(StatisticsRow("CSR", SC_SPH, MT_CPAP));

    rows.push_back(StatisticsRow(tr("Leak Statistics"),  SC_SUBHEADING, MT_CPAP));
    rows.push_back(StatisticsRow("Leak",       SC_WAVG,    MT_CPAP));
    rows.push_back(StatisticsRow("Leak",       SC_90P,     MT_CPAP));
    rows.push_back(StatisticsRow("Leak",       SC_ABOVE,   MT_CPAP));

    rows.push_back(StatisticsRow(tr("Pressure Statistics"),  SC_SUBHEADING, MT_CPAP));
    rows.push_back(StatisticsRow("Pressure",   SC_WAVG,    MT_CPAP));
    rows.push_back(StatisticsRow("Pressure",   SC_MIN,     MT_CPAP));
    rows.push_back(StatisticsRow("Pressure",   SC_MAX,     MT_CPAP));
    rows.push_back(StatisticsRow("Pressure",   SC_90P,     MT_CPAP));
    rows.push_back(StatisticsRow("EPAP",       SC_WAVG,    MT_CPAP));
    rows.push_back(StatisticsRow("EPAP",       SC_MIN,     MT_CPAP));
    rows.push_back(StatisticsRow("EPAP",       SC_MAX,     MT_CPAP));
    rows.push_back(StatisticsRow("IPAP",       SC_WAVG,    MT_CPAP));
    rows.push_back(StatisticsRow("IPAP",       SC_90P,     MT_CPAP));
    rows.push_back(StatisticsRow("IPAP",       SC_MIN,     MT_CPAP));
    rows.push_back(StatisticsRow("IPAP",       SC_MAX,     MT_CPAP));

    rows.push_back(StatisticsRow(tr("Oximeter Statistics"), SC_HEADING, MT_OXIMETER));
    rows.push_back(StatisticsRow("",           SC_DAYS,    MT_OXIMETER));
    rows.push_back(StatisticsRow("",           SC_COLUMNHEADERS, MT_OXIMETER));

    rows.push_back(StatisticsRow(tr("Blood Oxygen Saturation"),  SC_SUBHEADING, MT_CPAP));
    rows.push_back(StatisticsRow("SPO2",       SC_WAVG,     MT_OXIMETER));
    rows.push_back(StatisticsRow("SPO2",       SC_MIN,     MT_OXIMETER));
    rows.push_back(StatisticsRow("SPO2Drop",   SC_CPH,     MT_OXIMETER));
    rows.push_back(StatisticsRow("SPO2Drop",   SC_SPH,     MT_OXIMETER));
    rows.push_back(StatisticsRow(tr("Pulse Rate"),  SC_SUBHEADING, MT_CPAP));
    rows.push_back(StatisticsRow("Pulse",      SC_WAVG,     MT_OXIMETER));
    rows.push_back(StatisticsRow("Pulse",      SC_MIN,     MT_OXIMETER));
    rows.push_back(StatisticsRow("Pulse",      SC_MAX,     MT_OXIMETER));
    rows.push_back(StatisticsRow("PulseChange",   SC_CPH,     MT_OXIMETER));

    // These are for formatting the headers for the first column
    calcnames[SC_UNDEFINED] = "";
    calcnames[SC_MEDIAN] = tr("%1 Median");
    calcnames[SC_AVG] = tr("Average %1");
    calcnames[SC_WAVG] = tr("Average %1");
    calcnames[SC_90P] = tr("90% %1"); // this gets converted to whatever the upper percentile is set to
    calcnames[SC_MIN] = tr("Min %1");
    calcnames[SC_MAX] = tr("Max %1");
    calcnames[SC_CPH] = tr("%1 Index");
    calcnames[SC_SPH] = tr("% of time in %1");
    calcnames[SC_ABOVE] = tr("% of time above %1 threshold");
    calcnames[SC_BELOW] = tr("% of time below %1 threshold");

    machinenames[MT_UNKNOWN] = STR_TR_Unknown;
    machinenames[MT_CPAP] = STR_TR_CPAP;
    machinenames[MT_OXIMETER] = STR_TR_Oximetry;
    machinenames[MT_SLEEPSTAGE] = STR_TR_SleepStage;
//        { MT_JOURNAL, STR_TR_Journal },
//        { MT_POSITION, STR_TR_Position },

}

const QString table_width = "width=99%";
QString htmlHeader(bool showheader)
{

    QString address = p_profile->user->address();
    address.replace("\n", "<br/>");

    QString userinfo;

    if (!p_profile->user->firstName().isEmpty()) {
        userinfo = QString(QObject::tr("Name: %1, %2")).arg(p_profile->user->lastName()).arg(p_profile->user->firstName()) + "<br/>";
        if (!p_profile->user->DOB().isNull()) {
            userinfo += QString(QObject::tr("DOB: %1")).arg(p_profile->user->DOB().toString()) + "<br/>";
        }
        if (!p_profile->user->phone().isEmpty()) {
            userinfo += QString(QObject::tr("Phone: %1")).arg(p_profile->user->phone()) + "<br/>";
        }
        if (!p_profile->user->email().isEmpty()) {
            userinfo += QString(QObject::tr("Email: %1")).arg(p_profile->user->email()) + "<br/><br/>";
        }
        if (!p_profile->user->address().isEmpty()) {
            userinfo +=  QObject::tr("Address:")+"<br/>"+address;
        }
    }


    QString html = QString("<html><head>")+
    "</head>"
    "<style type='text/css'>"
    "p,a,td,body { font-family: '"+QApplication::font().family()+"'; }"
    "p,a,td,body { font-size: "+QString::number(QApplication::font().pointSize() + 2)+"px; }"

    "table.curved {"
    "border: 1px solid gray;"
    "border-radius:10px;"
    "-moz-border-radius:10px;"
    "-webkit-border-radius:10px;"
    "page-break-after:auto;"
    "-fs-table-paginate: paginate;"
    "}"
    "tr.datarow:nth-child(even) {"
    "background-color: #f8f8f8;"
    "}"
    "table { page-break-after:auto; -fs-table-paginate: paginate; }"
    "tr    { page-break-inside:avoid; page-break-after:auto }"
    "td    { page-break-inside:avoid; page-break-after:auto }"
    "thead { display:table-header-group; }"
    "tfoot { display:table-footer-group; }"

    "</style>"
    "<link rel='stylesheet' type='text/css' href='qrc:/docs/tooltips.css' />"
    "<script type='text/javascript'>"
    "function ChangeColor(tableRow, highLight)"
    "{ tableRow.style.backgroundColor = highLight; }"
    "function Go(url) { throw(url); }"
    "</script>"
    "</head>"
    "<body leftmargin=0 topmargin=5 rightmargin=0>";
    if (showheader) {
        html += "<div align=center><table class=curved "+table_width+">"
        "<td>"+userinfo+"</td>"
        "<td align='right'>"
        "<font size='+2'>" + STR_TR_SleepyHead + "</font><br/>"
        "<font size='+1'>" + QObject::tr("Usage Statistics") + "</font>"
        "</td>"
        "<td align='right' width=170px><img src='qrc:/icons/bob-v3.0.png' height=140px><br/>"
        "</td></tr></table>"
        "</div><br/>";
    }
    return html;
}
QString htmlFooter(bool showinfo=true)
{
    QString html;

    if (showinfo) {
        html += "<hr/><div align=center><font size='-1'><i>";
        html += QString(QObject::tr("This report was generated by a pre-release version of SleepyHead (%1), <b>and has not been approved in any way for compliance or medical diagnostic purposes</b>.")).
                arg(FullVersionString) + "<br/><br/>" +
           QObject::tr("SleepyHead is free open-source software available from http://sourceforge.net/projects/SleepyHead");
        html += "</i></font></div>";
    }

    html += "</body></html>";
    return html;
}



EventDataType calcAHI(QDate start, QDate end)
{
    EventDataType val = (p_profile->calcCount(CPAP_Obstructive, MT_CPAP, start, end)
                         + p_profile->calcCount(CPAP_Hypopnea, MT_CPAP, start, end)
                         + p_profile->calcCount(CPAP_ClearAirway, MT_CPAP, start, end)
                         + p_profile->calcCount(CPAP_Apnea, MT_CPAP, start, end));

    if (p_profile->general->calculateRDI()) {
        val += p_profile->calcCount(CPAP_RERA, MT_CPAP, start, end);
    }

    EventDataType hours = p_profile->calcHours(MT_CPAP, start, end);

    if (hours > 0) {
        val /= hours;
    } else {
        val = 0;
    }

    return val;
}

EventDataType calcFL(QDate start, QDate end)
{
    EventDataType val = (p_profile->calcCount(CPAP_FlowLimit, MT_CPAP, start, end));
    EventDataType hours = p_profile->calcHours(MT_CPAP, start, end);

    if (hours > 0) {
        val /= hours;
    } else {
        val = 0;
    }

    return val;
}

EventDataType calcSA(QDate start, QDate end)
{
    EventDataType val = (p_profile->calcCount(CPAP_SensAwake, MT_CPAP, start, end));
    EventDataType hours = p_profile->calcHours(MT_CPAP, start, end);

    if (hours > 0) {
        val /= hours;
    } else {
        val = 0;
    }

    return val;
}


struct RXChange {
    RXChange() { highlight = 0; machine = nullptr; }
    RXChange(const RXChange &copy) {
        first = copy.first;
        last = copy.last;
        days = copy.days;
        ahi = copy.ahi;
        fl = copy.fl;
        mode = copy.mode;
        min = copy.min;
        max = copy.max;
        ps = copy.ps;
        pshi = copy.pshi;
        maxipap = copy.maxipap;
        machine = copy.machine;
        per1 = copy.per1;
        per2 = copy.per2;
        highlight = copy.highlight;
        weighted = copy.weighted;
        pressure_string = copy.pressure_string;
        pr_relief_string = copy.pr_relief_string;
    }
    QDate first;
    QDate last;
    int days;
    EventDataType ahi;
    EventDataType fl;
    CPAPMode mode;
    QString pressure_string;
    QString pr_relief_string;
    EventDataType min;
    EventDataType max;
    EventDataType ps;
    EventDataType pshi;
    EventDataType maxipap;
    EventDataType per1;
    EventDataType per2;
    EventDataType weighted;
    Machine *machine;
    short highlight;
};

enum RXSortMode { RX_first, RX_last, RX_days, RX_ahi, RX_mode, RX_min, RX_max, RX_ps, RX_pshi, RX_maxipap, RX_per1, RX_per2, RX_weighted };
RXSortMode RXsort = RX_first;
bool RXorder = false;

bool operator<(const RXChange &c1, const RXChange &c2)
{
    const RXChange *comp1 = &c1;
    const RXChange *comp2 = &c2;

    if (RXorder) {
        switch (RXsort) {
        case RX_ahi:
            return comp1->ahi < comp2->ahi;

        case RX_days:
            return comp1->days < comp2->days;

        case RX_first:
            return comp1->first < comp2->first;

        case RX_last:
            return comp1->last < comp2->last;

        case RX_mode:
            return comp1->mode < comp2->mode;

        case RX_min:
            return comp1->min < comp2->min;

        case RX_max:
            return comp1->max < comp2->max;

        case RX_ps:
            return comp1->ps < comp2->ps;

        case RX_pshi:
            return comp1->pshi < comp2->pshi;

        case RX_maxipap:
            return comp1->maxipap < comp2->maxipap;

        case RX_per1:
            return comp1->per1 < comp2->per1;

        case RX_per2:
            return comp1->per2 < comp2->per2;

        case RX_weighted:
            return comp1->weighted < comp2->weighted;
        };
    } else {
        switch (RXsort) {
        case RX_ahi:
            return comp1->ahi > comp2->ahi;

        case RX_days:
            return comp1->days > comp2->days;

        case RX_first:
            return comp1->first > comp2->first;

        case RX_last:
            return comp1->last > comp2->last;

        case RX_mode:
            return comp1->mode > comp2->mode;

        case RX_min:
            return comp1->min > comp2->min;

        case RX_max:
            return comp1->max > comp2->max;

        case RX_ps:
            return comp1->ps > comp2->ps;

        case RX_pshi:
            return comp1->pshi > comp2->pshi;

        case RX_maxipap:
            return comp1->maxipap > comp2->maxipap;

        case RX_per1:
            return comp1->per1 > comp2->per1;

        case RX_per2:
            return comp1->per2 > comp2->per2;

        case RX_weighted:
            return comp1->weighted > comp2->weighted;
        };
    }

    return true;
}

bool RXSort(const RXChange *comp1, const RXChange *comp2)
{
    if (RXorder) {
        switch (RXsort) {
        case RX_ahi:
            return comp1->ahi < comp2->ahi;

        case RX_days:
            return comp1->days < comp2->days;

        case RX_first:
            return comp1->first < comp2->first;

        case RX_last:
            return comp1->last < comp2->last;

        case RX_mode:
            return comp1->mode < comp2->mode;

        case RX_min:
            return comp1->min < comp2->min;

        case RX_max:
            return comp1->max < comp2->max;

        case RX_ps:
            return comp1->ps < comp2->ps;

        case RX_pshi:
            return comp1->pshi < comp2->pshi;

        case RX_maxipap:
            return comp1->maxipap < comp2->maxipap;

        case RX_per1:
            return comp1->per1 < comp2->per1;

        case RX_per2:
            return comp1->per2 < comp2->per2;

        case RX_weighted:
            return comp1->weighted < comp2->weighted;
        };
    } else {
        switch (RXsort) {
        case RX_ahi:
            return comp1->ahi > comp2->ahi;

        case RX_days:
            return comp1->days > comp2->days;

        case RX_first:
            return comp1->first > comp2->first;

        case RX_last:
            return comp1->last > comp2->last;

        case RX_mode:
            return comp1->mode > comp2->mode;

        case RX_min:
            return comp1->min > comp2->min;

        case RX_max:
            return comp1->max > comp2->max;

        case RX_ps:
            return comp1->ps > comp2->ps;

        case RX_pshi:
            return comp1->pshi > comp2->pshi;

        case RX_maxipap:
            return comp1->maxipap > comp2->maxipap;

        case RX_per1:
            return comp1->per1 > comp2->per1;

        case RX_per2:
            return comp1->per2 > comp2->per2;

        case RX_weighted:
            return comp1->weighted > comp2->weighted;
        };
    }

    return true;
}
struct UsageData {
    UsageData() { ahi = 0; hours = 0; }
    UsageData(QDate d, EventDataType v, EventDataType h) { date = d; ahi = v; hours = h; }
    UsageData(const UsageData &copy) { date = copy.date; ahi = copy.ahi; hours = copy.hours; }
    QDate date;
    EventDataType ahi;
    EventDataType hours;
};
bool operator <(const UsageData &c1, const UsageData &c2)
{
    if (c1.ahi < c2.ahi) {
        return true;
    }

    if ((c1.ahi == c2.ahi) && (c1.date > c2.date)) { return true; }

    return false;
    //return c1.value < c2.value;
}

struct Period {
    Period() {
    }
    Period(QDate start, QDate end, QString header) {
        this->start = start;
        this->end = end;
        this->header = header;
    }
    Period(const Period & copy) {
        start=copy.start;
        end=copy.end;
        header=copy.header;
    }
    QDate start;
    QDate end;
    QString header;
};

QString Statistics::GenerateHTML()
{

    QString heading_color="#ffffff";
    QString subheading_color="#e0e0e0";


    // Find first and last days with valid CPAP data
    QDate lastcpap = p_profile->LastGoodDay(MT_CPAP);
    QDate firstcpap = p_profile->FirstGoodDay(MT_CPAP);


    QDate cpapweek = lastcpap.addDays(-6);
    QDate cpapmonth = lastcpap.addDays(-29);
    QDate cpap6month = lastcpap.addMonths(-6);
    QDate cpapyear = lastcpap.addMonths(-12);

    if (cpapweek < firstcpap) { cpapweek = firstcpap; }
    if (cpapmonth < firstcpap) { cpapmonth = firstcpap; }
    if (cpap6month < firstcpap) { cpap6month = firstcpap; }
    if (cpapyear < firstcpap) { cpapyear = firstcpap; }

    QList<Machine *> cpap_machines = p_profile->GetMachines(MT_CPAP);
    QList<Machine *> oximeters = p_profile->GetMachines(MT_OXIMETER);
    QList<Machine *> mach;

    mach.append(cpap_machines);
    mach.append(oximeters);

    bool havedata = false;
    for (int i=0; i < mach.size(); ++i) {
        int daysize = mach[i]->day.size();
        if (daysize > 0) {
            havedata = true;
            break;
        }
    }

    QString html = htmlHeader(havedata);

    if (!havedata) {
        html += "<div align=center><table class=curved height=100% "+table_width+">";
        html += QString("<tr><td colspan=2 align=center>") +
                "<img src='qrc:/icons/bob-v3.0.png' height=100px>"
                "<p><font size=+3>" + tr("I can haz data?!?") + "</font></p>"+
                "<p><i>"+tr("This lonely sleepy sheep has no data :(")+"</i></p>"

        "</table></div>";
        html += htmlFooter(havedata);
        return html;
    }

    int cpapdays = p_profile->countDays(MT_CPAP, firstcpap, lastcpap);

//    CPAPMode cpapmode = (CPAPMode)(int)p_profile->calcSettingsMax(CPAP_Mode, MT_CPAP, firstcpap, lastcpap);

    float percentile = p_profile->general->prefCalcPercentile() / 100.0;

    //    int mididx=p_profile->general->prefCalcMiddle();
    //    SummaryType ST_mid;
    //    if (mididx==0) ST_mid=ST_PERC;
    //    if (mididx==1) ST_mid=ST_WAVG;
    //    if (mididx==2) ST_mid=ST_AVG;

    QString ahitxt;

    if (p_profile->general->calculateRDI()) {
        ahitxt = STR_TR_RDI;
    } else {
        ahitxt = STR_TR_AHI;
    }

    int decimals = 2;
    html += "<div align=center>";
    html += QString("<table class=curved "+table_width+">");

    int number_periods = 0;
    if (p_profile->general->statReportMode() == 1) {
        number_periods = p_profile->FirstDay().daysTo(p_profile->LastDay()) / 30;
        if (number_periods > 12) {
            number_periods = 12;
        }
    }

    QDate last = lastcpap, first = lastcpap;

    QList<Period> periods;



    bool skipsection = false;;
    for (QList<StatisticsRow>::iterator i = rows.begin(); i != rows.end(); ++i) {
        StatisticsRow &row = (*i);
        QString name;

        if (row.calc == SC_HEADING) {  // All sections begin with a heading
            last = p_profile->LastGoodDay(row.type);
            first = p_profile->FirstGoodDay(row.type);

            periods.clear();
            if (p_profile->general->statReportMode() == 0) {
                periods.push_back(Period(last,last,tr("Most Recent")));
                periods.push_back(Period(qMax(last.addDays(-6), first), last, tr("Last Week")));
                periods.push_back(Period(qMax(last.addDays(-29),first), last, tr("Last 30 Days")));
                periods.push_back(Period(qMax(last.addMonths(-6), first), last, tr("Last 6 Months")));
                periods.push_back(Period(qMax(last.addMonths(-12), first), last, tr("Last Year")));
            } else {
                QDate l=last,s=last;

                periods.push_back(Period(last,last,tr("Last Session")));

                bool done=false;
                int j=0;
                do {
                    s=QDate(l.year(), l.month(), 1);
                    if (s < first) {
                        done = true;
                        s = first;
                    }
                    if (p_profile->countDays(row.type, s, l) > 0) {
                        periods.push_back(Period(s, l, s.toString("MMMM")));
                        j++;
                    }
                    l = s.addDays(-1);
                } while ((l > first) && (j < number_periods));

//                for (; j < number_periods; ++j) {
//                    s=QDate(l.year(), l.month(), 1);
//                    if (s < first) {
//                        done = true;
//                        s = first;
//                    }
//                    if (p_profile->countDays(row.type, s, l) > 0) {
//                        periods.push_back(Period(s, l, s.toString("MMMM")));
//                    } else {
//                    }
//                    l = s.addDays(-1);
//                    if (done || (l < first)) break;
//                }
                for (; j < number_periods; ++j) {
                    periods.push_back(Period(last,last, ""));
                }
            }

            int days = p_profile->countDays(row.type, first, last);
            skipsection = (days == 0);
            if (days > 0) {
                html+=QString("<tr bgcolor='%1'><th colspan=%2 align=center><font size=+2>%3</font></th></tr>\n").
                        arg(heading_color).arg(periods.size()+1).arg(row.src);
            }
            continue;
        }

        // Bypass this entire section if no data is present
        if (skipsection) continue;

        if (row.calc == SC_AHI) {
            name = ahitxt;
        } else if ((row.calc == SC_HOURS) || (row.calc == SC_COMPLIANCE)) {
            name = row.src;
        } else if (row.calc == SC_COLUMNHEADERS) {
            html += QString("<tr><td><b>%1</b></td>\n").arg(tr("Details"));
            for (int j=0; j < periods.size(); j++) {
                html += QString("<td onmouseover='ChangeColor(this, \"#eeeeee\");' onmouseout='ChangeColor(this, \"#ffffff\");' onclick='alert(\"overview=%1,%2\");'><b>%3</b></td>\n").arg(periods.at(j).start.toString(Qt::ISODate)).arg(periods.at(j).end.toString(Qt::ISODate)).arg(periods.at(j).header);
            }
            html += "</tr>\n";
            continue;
        } else if (row.calc == SC_DAYS) {
            QDate first=p_profile->FirstGoodDay(row.type);
            QDate last=p_profile->LastGoodDay(row.type);
            QString & machine = machinenames[row.type];
            int value=p_profile->countDays(row.type, first, last);

            if (value == 0) {
                html+=QString("<tr><td colspan=%1 align=center>%2</td></tr>\n").arg(periods.size()+1).
                        arg(QString(tr("No %1 data available.")).arg(machine));
            } else if (value == 1) {
                html+=QString("<tr><td colspan=%1 align=center>%2</td></tr>\n").arg(periods.size()+1).
                        arg(QString(tr("%1 day of %2 Data on %3"))
                            .arg(value)
                            .arg(machine)
                            .arg(last.toString()));
            } else {
                html+=QString("<tr><td colspan=%1 align=center>%2</td></tr>\n").arg(periods.size()+1).
                        arg(QString(tr("%1 days of %2 Data, between %3 and %4"))
                            .arg(value)
                            .arg(machine)
                            .arg(first.toString())
                            .arg(last.toString()));
            }
            continue;
        } else if (row.calc == SC_SUBHEADING) {  // subheading..
            html+=QString("<tr bgcolor='%1'><td colspan=%2 align=center><b>%3</b></td></tr>\n").
                    arg(subheading_color).arg(periods.size()+1).arg(row.src);
            continue;
        } else if (row.calc == SC_UNDEFINED) {
            continue;
        } else {
            ChannelID id = schema::channel[row.src].id();
            if ((id == NoChannel) || (!p_profile->hasChannel(id))) {
                continue;
            }
            name = calcnames[row.calc].arg(schema::channel[id].fullname());
        }
        QString line;
        line += QString("<tr class=datarow><td width=25%>%1</td>").arg(name);
        int np = periods.size();
        int width;
        for (int j=0; j < np; j++) {
            if (p_profile->general->statReportMode() == 1) {
                width = j < np-1 ? 6 : 100 - (25 + 6*(np-1));
            } else {
                width = 75/np;
            }

            line += QString("<td width=%1%>").arg(width);
            if (!periods.at(j).header.isEmpty()) {
                line += row.value(periods.at(j).start, periods.at(j).end);
            } else {
                line +="&nbsp;";
            }
            line += "</td>";
        }
        html += line;
        html += "</tr>\n";
    }

    html += "</table>";
    html += "</div>";

    QList<UsageData> AHI;

    if (cpapdays > 0) {
        QDate first, last = lastcpap;
        CPAPMode mode = MODE_UNKNOWN, cmode = MODE_UNKNOWN;
        EventDataType cmin = 0, cmax = 0, cps = 0, cpshi = 0, cmaxipap = 0, min = 0, max = 0, maxipap = 0,
                      ps = 0, pshi = 0;
        Machine *mach = nullptr, *lastmach = nullptr;
        QString last_prel_str, last_pressure_str, prel_str, pressure_str;
        QDate date = lastcpap;
        Day *day;
        bool lastchanged = false;
        QVector<RXChange> rxchange;
        EventDataType hours;

        int compliant = 0;

        do {
            day = p_profile->GetGoodDay(date, MT_CPAP);


            CPAPLoader * loader = nullptr;

            if (day) loader = dynamic_cast<CPAPLoader *>(day->machine->loader());

            if (day && loader) {
                lastchanged = false;

                hours = day->hours();

                if (hours > p_profile->cpap->complianceHours()) {
                    compliant++;
                }

                EventDataType ahi = day->count(CPAP_Obstructive) + day->count(CPAP_Hypopnea) + day->count(CPAP_Apnea) + day->count(CPAP_ClearAirway);

                if (p_profile->general->calculateRDI()) { ahi += day->count(CPAP_RERA); }

                ahi /= hours;
                AHI.push_back(UsageData(date, ahi, hours));

                prel_str = day->getPressureRelief();
                pressure_str = day->getPressureSettings();

                mode = (CPAPMode)(int)round(day->settings_wavg(CPAP_Mode));
                if (mode ==0) {
                    mode = (CPAPMode)(int)round(day->settings_wavg(CPAP_Mode));
                }
                mach = day->machine;

                min = max = ps = pshi = maxipap = 0;

                if (mode == MODE_CPAP) {
                    min = day->settings_min(CPAP_Pressure);
                } else if (mode == MODE_APAP) {
                    min = day->settings_min(CPAP_PressureMin);
                    max = day->settings_max(CPAP_PressureMax);
                } else if (mode ==  MODE_BILEVEL_FIXED) {
                    min = day->settings_min(CPAP_EPAP);
                    max = day->settings_max(CPAP_IPAP);
                    ps = max-min;
                } else if (mode == MODE_BILEVEL_AUTO_FIXED_PS) {
                    min = day->settings_min(CPAP_EPAPLo);
                    maxipap = max = day->settings_max(CPAP_IPAPHi);
                    ps = day->settings_min(CPAP_PS);
                } else if (mode == MODE_BILEVEL_AUTO_VARIABLE_PS) { // Similar pressure control as ASV Variable EPAP
                    min = day->settings_min(CPAP_EPAPLo);
                    max = day->settings_min(CPAP_EPAPHi);
                    ps = day->settings_min(CPAP_PSMin);
                    pshi = day->settings_max(CPAP_PSMax);
                    maxipap = max = day->settings_max(CPAP_IPAPHi);
                } else if (mode ==  MODE_ASV) {
                    min = day->settings_min(CPAP_EPAPLo);
                    ps = day->settings_min(CPAP_PSMin);
                    pshi = day->settings_max(CPAP_PSMax);
                    maxipap = min+pshi;
                } else if (mode == MODE_ASV_VARIABLE_EPAP) {
                    min = day->settings_min(CPAP_EPAPLo);
                    max = day->settings_max(CPAP_EPAPHi);
                    ps = day->settings_min(CPAP_PSMin);
                    pshi = day->settings_max(CPAP_PSMax);
                    maxipap = max + pshi;

                }

                if (lastmach == nullptr) {
                    lastmach = mach;
                    cmode = mode;
                    last_pressure_str = pressure_str;
                    last_prel_str = prel_str;

                }

                if ((mode != cmode) || (pressure_str != last_pressure_str) || (prel_str != last_prel_str) || (mach != lastmach)) {
                    first = date.addDays(1);
                    int days = p_profile->countDays(MT_CPAP, first, last);
                    RXChange rx;
                    rx.first = first;
                    rx.last = last;
                    rx.days = days;
                    rx.ahi = calcAHI(first, last);
                    rx.fl = calcFL(first, last);
                    rx.mode = cmode;
                    rx.pressure_string = last_pressure_str;
                    rx.pr_relief_string = last_prel_str;
                    rx.min = cmin;
                    rx.max = cmax;
                    rx.ps = cps;
                    rx.pshi = cpshi;
                    rx.maxipap = cmaxipap;
                    rx.machine = lastmach;
                    rx.per1 = 0;
                    rx.per2 = 0;

                    if (mode == MODE_APAP) {
                        rx.per1 = p_profile->calcPercentile(CPAP_Pressure, percentile, MT_CPAP, first, last);
                        rx.per2 = 0;
                    } else if (mode == MODE_BILEVEL_AUTO_FIXED_PS) {
                        rx.per1 = p_profile->calcPercentile(CPAP_EPAP, percentile, MT_CPAP, first, last);
                        rx.per2 = p_profile->calcPercentile(CPAP_IPAP, percentile, MT_CPAP, first, last);
                    } else if (mode == MODE_BILEVEL_AUTO_VARIABLE_PS) {
                        rx.per1 = p_profile->calcPercentile(CPAP_EPAP, percentile, MT_CPAP, first, last);
                        rx.per2 = p_profile->calcPercentile(CPAP_PS, percentile, MT_CPAP, first, last);
                    } else if (mode == MODE_ASV) {
                        rx.per1 = p_profile->calcPercentile(CPAP_PS, percentile, MT_CPAP, first, last);
                        rx.per2 = p_profile->calcPercentile(CPAP_PS, percentile, MT_CPAP, first, last);
                    } else if (mode == MODE_ASV_VARIABLE_EPAP) {
                        rx.per1 = p_profile->calcPercentile(CPAP_EPAP, percentile, MT_CPAP, first, last);
                        rx.per2 = p_profile->calcPercentile(CPAP_PS, percentile, MT_CPAP, first, last);
                    }

                    rx.weighted = float(rx.days) / float(cpapdays) * rx.ahi;
                    rxchange.push_back(rx);

                    cmode = mode;
                    cmin = min;
                    cmax = max;
                    cps = ps;
                    cpshi = pshi;
                    cmaxipap = maxipap;
                    last_prel_str = prel_str;
                    last_pressure_str = pressure_str;
                    last = date;
                    lastmach = mach;
                    lastchanged = true;
                }

            }

            date = date.addDays(-1);
        } while (date >= firstcpap);

        // Sort list by AHI
        qSort(AHI);

       lastchanged = false;

        // Add the final entry
        if (!lastchanged && (mach != nullptr)) {
            // last=date.addDays(1);
            first = firstcpap;
            int days = p_profile->countDays(MT_CPAP, first, last);
            RXChange rx;
            rx.first = first;
            rx.last = last;
            rx.days = days;
            rx.ahi = calcAHI(first, last);
            rx.fl = calcFL(first, last);
            rx.mode = mode;
            rx.min = min;
            rx.max = max;
            rx.ps = ps;
            rx.pshi = pshi;
            rx.maxipap = maxipap;
            rx.machine = mach;
            rx.pressure_string = pressure_str;
            rx.pr_relief_string = prel_str;

            // Todo: Clean up by Calculating this crap later..
            if (mode == MODE_APAP) {
                rx.per1 = p_profile->calcPercentile(CPAP_Pressure, percentile, MT_CPAP, first, last);
                rx.per2 = 0;
            } else if (mode == MODE_BILEVEL_AUTO_FIXED_PS) {
                rx.per1 = p_profile->calcPercentile(CPAP_EPAP, percentile, MT_CPAP, first, last);
                rx.per2 = p_profile->calcPercentile(CPAP_IPAP, percentile, MT_CPAP, first, last);
            } else if (mode == MODE_BILEVEL_AUTO_VARIABLE_PS) {
                rx.per1 = p_profile->calcPercentile(CPAP_EPAP, percentile, MT_CPAP, first, last);
                rx.per2 = p_profile->calcPercentile(CPAP_PS, percentile, MT_CPAP, first, last);
            } else if (mode == MODE_ASV) {
                rx.per1 = p_profile->calcPercentile(CPAP_PS, percentile, MT_CPAP, first, last);
                rx.per2 = p_profile->calcPercentile(CPAP_PS, percentile, MT_CPAP, first, last);
            } else if (mode == MODE_ASV_VARIABLE_EPAP) {
                rx.per1 = p_profile->calcPercentile(CPAP_EPAP, percentile, MT_CPAP, first, last);
                rx.per2 = p_profile->calcPercentile(CPAP_PS, percentile, MT_CPAP, first, last);
            }

            rx.weighted = float(rx.days) / float(cpapdays);
            //rx.weighted=float(days)*rx.ahi;
            rxchange.push_back(rx);
        }

        int rxthresh = 5;
        QVector<RXChange *> tmpRX;

        for (int i = 0; i < rxchange.size(); i++) {
            RXChange &rx = rxchange[i];

            if (rx.days >= rxthresh) {
                tmpRX.push_back(&rx);
            }
        }

        QString recbox = "<html><head><style type='text/css'>"
                         "p,a,td,body { font-family: '" + QApplication::font().family() + "'; }"
                         "p,a,td,body { font-size: " + QString::number(QApplication::font().pointSize() + 2) + "px; }"
                         "a:link,a:visited { color: inherit; text-decoration: none; }" //font-weight: normal;
                         "a:hover { background-color: inherit; color: white; text-decoration:none; font-weight: bold; }"
                         "</style></head><body>";
        recbox += "<table width=100% cellpadding=1 cellspacing=0>";
        int numdays = AHI.size();

        if (numdays > 1) {
            int z = numdays / 2;

            if (z > 4) { z = 4; }

            recbox += QString("<tr><td colspan=2 align=center><b>%1</b></td></tr>").arg(
                          tr("Usage Information"));
            recbox += QString("<tr><td>%1</td><td align=right>%2</td></tr>").arg(tr("Total Days")).arg(
                          numdays);

            if (p_profile->cpap->showComplianceInfo()) {
                recbox += QString("<tr><td>%1</td><td align=right>%2</td></tr>").arg(tr("Compliant Days")).arg(
                              compliant);
            }

            int highahi = 0;

            for (int i = 0; i < numdays; i++) {
                if (AHI.at(i).ahi > 5.0) {
                    highahi++;
                }
            }

            recbox += QString("<tr><td>%1</td><td align=right>%2</td></tr>").arg(tr("Days AHI &gt;5.0")).arg(
                          highahi);


            recbox += QString("<tr><td colspan=2>&nbsp;</td></tr>");
            recbox += QString("<tr><td colspan=2 align=center><b>%1</b></td></tr>").arg(tr("Best&nbsp;%1").arg(
                          ahitxt));

            for (int i = 0; i < z; i++) {
                const UsageData &a = AHI.at(i);
                recbox += QString("<tr><td><a href='daily=%1'>%2</a></td><td  align=right>%3</td></tr>")
                          .arg(a.date.toString(Qt::ISODate))
                          .arg(a.date.toString(Qt::SystemLocaleShortDate))
                          .arg(a.ahi, 0, 'f', decimals);
            }

            recbox += QString("<tr><td colspan=2>&nbsp;</td></tr>");
            recbox += QString("<tr><td colspan=2 align=center><b>%1</b></td></tr>").arg(
                          tr("Worst&nbsp;%1").arg(ahitxt));

            for (int i = 0; i < z; i++) {
                const UsageData &a = AHI.at((numdays - 1) - i);
                recbox += QString("<tr><td><a href='daily=%1'>%2</a></td><td align=right>%3</td></tr>")
                          .arg(a.date.toString(Qt::ISODate))
                          .arg(a.date.toString(Qt::SystemLocaleShortDate))
                          .arg(a.ahi, 0, 'f', decimals);
            }

            recbox += QString("<tr><td colspan=2>&nbsp;</td></tr>");
        }


        if (tmpRX.size() > 0) {
            RXsort = RX_ahi;
            qSort(tmpRX.begin(), tmpRX.end(), RXSort);
            tmpRX[0]->highlight = 4; // worst
            int ls = tmpRX.size() - 1;
            tmpRX[ls]->highlight = 1; //best
            QString modestr;

            QString idxstr[2];
            int idx[2];
            idx[0]=ls;
            idx[1]=0;
            idxstr[0]=tr("Best RX Setting");
            idxstr[1]=tr("Worst RX Setting");

            for (int i=0; i<2; ++i ) {
                int i2 = idx[i];
                if ((i==1) && (ls == 0)) break;
                CPAPMode mode = (CPAPMode)(int)p_profile->calcSettingsMax(CPAP_Mode, MT_CPAP, tmpRX[i2]->first, tmpRX[i2]->last);

                if (mode == MODE_CPAP) {
                    modestr = STR_TR_CPAP;
                } else if (mode ==  MODE_APAP) { // is AUTO?
                    modestr = STR_TR_APAP;
                } else if (mode == MODE_BILEVEL_FIXED) {
                    modestr = STR_TR_BiLevel;
                } else if (mode == MODE_BILEVEL_AUTO_FIXED_PS) {
                    modestr = QObject::tr("Auto Bi-Level");
                } else if (mode == MODE_BILEVEL_AUTO_VARIABLE_PS) {
                    modestr = QObject::tr("Auto Bi-Level");
                } else if (mode == MODE_ASV) {
                    modestr = QObject::tr("ASV Fixed EPAP");
                } else if (mode == MODE_ASV_VARIABLE_EPAP) {
                    modestr = QObject::tr("ASV Auto EPAP");
                } else modestr = STR_TR_Unknown;


                recbox += QString("<tr><td colspan=2><table width=100% border=0 cellpadding=1 cellspacing=0><tr><td colspan=2 align=center><b>%3</b></td></tr>")
                          .arg(idxstr[i]);
                recbox += QString("<tr><td valign=top>") + STR_TR_Start + "<br/>" + STR_TR_End +
                          QString("</td><td align=right><a href='overview=%1,%2'>%3<br/>%4</a></td></tr>")
                          .arg(tmpRX[i2]->first.toString(Qt::ISODate))
                          .arg(tmpRX[i2]->last.toString(Qt::ISODate))
                          .arg(tmpRX[i2]->first.toString(Qt::SystemLocaleShortDate))
                          .arg(tmpRX[i2]->last.toString(Qt::SystemLocaleShortDate));
                recbox += QString("<tr><td><b>%1</b></td><td align=right><b>%2</b></td></tr>").arg(ahitxt).arg(tmpRX[i2]->ahi, 0, 'f', decimals);
                recbox += QString("<tr><td>%1</td><td align=right>%2</td></tr>").arg(STR_TR_Mode).arg(modestr);
                if (mode == MODE_CPAP) {
                    recbox += QString("<tr><td>")+QObject::tr("Fixed")+QString("</td><td align=right>%1%2</td></tr>").arg(tmpRX[i2]->min, 0, 'f', 1).arg(STR_UNIT_CMH2O);
                } else if (mode == MODE_APAP) {
                    recbox += QString("<tr><td>")+QObject::tr("Min")+QString("</td><td align=right>%1%2</td></tr>").arg(tmpRX[i2]->min, 0, 'f', 1).arg(STR_UNIT_CMH2O);
                    recbox += QString("<tr><td>")+QObject::tr("Max")+QString("</td><td align=right>%1%2</td></tr>").arg(tmpRX[i2]->max, 0, 'f', 1).arg(STR_UNIT_CMH2O);
                } else if (mode == MODE_BILEVEL_FIXED) {
                    recbox += QString("<tr><td>")+QObject::tr("EPAP")+QString("</td><td align=right>%1%2</td></tr>").arg(tmpRX[i2]->min, 0, 'f', 1).arg(STR_UNIT_CMH2O);
                    recbox += QString("<tr><td>")+QObject::tr("IPAP")+QString("</td><td align=right>%1%2</td></tr>").arg(tmpRX[i2]->max, 0, 'f', 1).arg(STR_UNIT_CMH2O);
                } else if (mode == MODE_BILEVEL_AUTO_FIXED_PS) {
                    recbox += QString("<tr><td>")+QObject::tr("Min EPAP")+QString("</td><td align=right>%1%2</td></tr>").arg(tmpRX[i2]->min, 0, 'f', 1).arg(STR_UNIT_CMH2O);
                    recbox += QString("<tr><td>")+QObject::tr("Max IPAP")+QString("</td><td align=right>%1%2</td></tr>").arg(tmpRX[i2]->max, 0, 'f', 1).arg(STR_UNIT_CMH2O);
                    recbox += QString("<tr><td>")+QObject::tr("PS")+QString("</td><td align=right>%1%2</td></tr>").arg(tmpRX[i2]->ps, 0, 'f', 1).arg(STR_UNIT_CMH2O);
                } else if (mode == MODE_ASV) {
                    recbox += QString("<tr><td>")+QObject::tr("EPAP")+QString("</td><td align=right>%1%2</td></tr>").arg(tmpRX[i2]->min, 0, 'f', 1).arg(STR_UNIT_CMH2O);
                    recbox += QString("<tr><td>")+QObject::tr("PS")+QString("</td><td align=right>%1-%2%3</td></tr>").arg(tmpRX[i2]->ps, 0, 'f', 1).arg(tmpRX[i2]->pshi, 0, 'f', 1).arg(STR_UNIT_CMH2O);
//                    recbox += QString("<tr><td>")+QObject::tr("Max&nbsp;IPAP")+QString("</td><td align=right>%1%2</td></tr>").arg(tmpRX[i2]->maxipap, 0, 'f', 1).arg(STR_UNIT_CMH2O);
                } else if (mode == MODE_ASV_VARIABLE_EPAP) {
                    recbox += QString("<tr><td>")+QObject::tr("EPAP")+QString("</td><td align=right>%1-%2%3</td></tr>").arg(tmpRX[i2]->min, 0, 'f', 1).arg(tmpRX[i2]->max, 0, 'f', 1).arg(STR_UNIT_CMH2O);
                    recbox += QString("<tr><td>")+QObject::tr("PS")+QString("</td><td align=right>%1-%2%3</td></tr>").arg(tmpRX[i2]->ps, 0, 'f', 1).arg(tmpRX[i2]->pshi, 0, 'f', 1).arg(STR_UNIT_CMH2O);
//                    recbox += QString("<tr><td>")+QObject::tr("Max&nbsp;IPAP")+QString("</td><td align=right>%1%2</td></tr>").arg(tmpRX[i2]->maxipap, 0, 'f', 1).arg(STR_UNIT_CMH2O);
                }
                recbox += "</table><br/></td></tr>";
            }
        }

        recbox += "</table>";
        recbox += "</body></html>";
        mainwin->setRecBoxHTML(recbox);

        /*RXsort=RX_min;
        RXorder=true;
        qSort(rxchange.begin(),rxchange.end());*/

        html += "<div align=center><br/>";
        html += QString("<table class=curved style=\"page-break-before:always;\" "+table_width+">");
        html += "<thead>";
        html += "<tr bgcolor='"+heading_color+"'><th colspan=10 align=center><font size=+2>" + tr("Changes to Prescription Settings") + "</font></th></tr>";

        QString extratxt;

        QString tooltip;
        QStringList hdrlist;
        hdrlist.push_back(STR_TR_First);
        hdrlist.push_back(STR_TR_Last);
        hdrlist.push_back(tr("Days"));
        hdrlist.push_back(ahitxt);
        hdrlist.push_back(STR_TR_FL);
        if (p_profile->hasChannel(CPAP_SensAwake)) {
            hdrlist.push_back(STR_TR_SA);
        }
        hdrlist.push_back(STR_TR_Machine);
        hdrlist.push_back(tr("Pressure Relief"));
        hdrlist.push_back(STR_TR_Mode);
        hdrlist.push_back(tr("Pressure Settings"));

        html+="<tr>\n";
        for (int i=0; i < hdrlist.size(); ++i) {
            html+=QString(" <th align=left><b>%1</b></th>\n").arg(hdrlist.at(i));
        }
        html+="</tr>\n";
        html += "</thead>";
        html += "<tfoot>";
        html += "<tr><td colspan=10 align=center>";
        html += QString("<i>") +
                tr("Efficacy highlighting ignores prescription settings with less than %1 days of recorded data.").
                arg(rxthresh) + QString("</i><br/>");

        html += "</td></tr>";
        html += "</tfoot>";


        for (int i = 0; i < rxchange.size(); i++) {
            RXChange rx = rxchange.at(i);
            QString color;

            if (rx.highlight == 1) {
                color = "#c0ffc0";
            } else if (rx.highlight == 2) {
                color = "#e0ffe0";
            } else if (rx.highlight == 3) {
                color = "#ffe0e0";
            } else if (rx.highlight == 4) {
                color = "#ffc0c0";
            } else { color = ""; }

            QString machstr;

            if (!rx.machine->brand().isEmpty()) {
                machstr += rx.machine->brand();
            }

            if (!rx.machine->model().isEmpty()) {
                machstr += " " + rx.machine->model();
            }

            if (!rx.machine->serial().isEmpty()) {
                machstr += " (" + rx.machine->serial() + ")<br/>";
            }

            mode = rx.mode;
            extratxt = "<table border=0 width=100%><tr>"; //cellpadding=0 cellspacing=0

            //            tooltip=QString("%1 %2% ").arg(machstr).arg(percentile*100.0)+STR_TR_EPAP+
            //                    QString("=%1<br/>%2% ").arg(rx.per1,0,'f',decimals).arg(percentile*100.0)+
            //                    STR_TR_IPAP+QString("=%1").arg(rx.per2,0,'f',decimals);
            tooltip = QString("%1").arg(machstr);

            extratxt += "<td colspan=2>"+rx.pressure_string+"</td>";
//            if (mode == MODE_CPAP) {
//                extratxt += "<td colspan=2>"+QString(tr("Fixed %1 %2")+"</td>").arg(rx.min, 4, 'f', 1).arg(schema::channel[CPAP_Pressure].units());
//            } else if (mode == MODE_APAP) {
//                extratxt += "<td colspan=2>"+QString(tr("%1 - %2 %3")+"</td>").arg(rx.min, 4, 'f', 1).arg(rx.max, 4, 'f', 1).arg(schema::channel[CPAP_Pressure].units());
//            } else if (mode == MODE_BILEVEL_FIXED) {
//                extratxt += "<td colspan=2>"+QString(tr("EPAP %1 %3 IPAP %2 %3")+"</td>").arg(rx.min, 4, 'f', 1).arg(rx.max, 4, 'f', 1).arg(schema::channel[CPAP_Pressure].units());
//            } else if (mode == MODE_BILEVEL_AUTO_FIXED_PS) {
//                extratxt += "<td colspan=2>"+QString(tr("PS %4 over %1 - %2 %3")+"</td>").arg(rx.min, 4, 'f', 1).arg(rx.max, 4, 'f', 1).arg(schema::channel[CPAP_Pressure].units()).arg(rx.ps, 4, 'f', 1);
//            } else if (mode == MODE_BILEVEL_AUTO_VARIABLE_PS) {
//                extratxt += "<td colspan=2>"+QString(tr("EPAP %1-%2, PS %3-%4 %5")+"</td>").arg(rx.min, 4, 'f', 1).arg(rx.max, 4, 'f', 1).arg(rx.ps, 4, 'f', 1).arg(rx.pshi, 4, 'f', 1).arg(schema::channel[CPAP_Pressure].units());
//            } else if (mode == MODE_ASV) {
//                extratxt += "<td colspan=2>"+QString(tr("EPAP %1, PS %2-%3 %4")+"</td>").arg(rx.min, 4, 'f', 1).arg(rx.ps, 4, 'f', 1).arg(rx.pshi, 4, 'f', 1).arg(schema::channel[CPAP_Pressure].units());
//                tooltip = QString("%1 %2% ").arg(machstr).arg(percentile * 100.0) +
//                          STR_TR_EPAP +
//                          QString("=%1<br/>%2% ").arg(rx.per1, 0, 'f', decimals)
//                          .arg(percentile * 100.0)
//                          + STR_TR_IPAP + QString("=%1").arg(rx.per2, 0, 'f', decimals);
//            } else if (mode == MODE_ASV_VARIABLE_EPAP) {
//                extratxt += "<td colspan=2>"+QString(tr("EPAP %1-%2, PS %3-%4 %5")+"</td>").arg(rx.min, 4, 'f', 1).arg(rx.max, 4, 'f', 1).arg(rx.ps, 4, 'f', 1).arg(rx.pshi, 4, 'f', 1).arg(schema::channel[CPAP_Pressure].units());
//                tooltip = QString("%1 %2% ").arg(machstr).arg(percentile * 100.0) +
//                          STR_TR_EPAP +
//                          QString("=%1<br/>%2% ").arg(rx.per1, 0, 'f', decimals)
//                          .arg(percentile * 100.0)
//                          + STR_TR_IPAP + QString("=%1").arg(rx.per2, 0, 'f', decimals);
//            } else {
//                    extratxt += "";
//                    tooltip = "";
//            }

            extratxt += "</tr></table>";

            QString tooltipshow, tooltiphide;

            if (!tooltip.isEmpty()) {
                tooltipshow = QString("tooltip.show(\"%1\");").arg(tooltip);
                tooltiphide = "tooltip.hide();";
            }

            QString datarowclass;
            if (rx.highlight == 0) datarowclass="class=datarow";
            html += QString("<tr %6 bgcolor='%1' onmouseover='ChangeColor(this, \"#eeeeee\"); %2' onmouseout='ChangeColor(this, \"%1\"); %3' onclick='alert(\"overview=%4,%5\");'>")
                    .arg(color)
                    .arg(tooltipshow)
                    .arg(tooltiphide)
                    .arg(rx.first.toString(Qt::ISODate))
                    .arg(rx.last.toString(Qt::ISODate))
                    .arg(datarowclass);


            html += QString("<td>%1</td>").arg(rx.first.toString(Qt::SystemLocaleShortDate));
            html += QString("<td>%1</td>").arg(rx.last.toString(Qt::SystemLocaleShortDate));
            html += QString("<td>%1</td>").arg(rx.days);
            html += QString("<td>%1</td>").arg(rx.ahi, 0, 'f', decimals);
            html += QString("<td>%1</td>").arg(rx.fl, 0, 'f', decimals); // Not the best way to do this.. Todo: Add an extra field for data..

            if (p_profile->hasChannel(CPAP_SensAwake)) {
                html += QString("<td>%1</td>").arg(calcSA(rx.first, rx.last), 0, 'f', decimals);
            }
            html += QString("<td>%1</td>").arg(rx.machine->loaderName());
            html += QString("<td>%1</td>").arg(rx.pr_relief_string);
            html += QString("<td>%1</td>").arg(schema::channel[CPAP_Mode].option(int(rx.mode)));
            html += QString("<td>%1</td>").arg(extratxt);
            html += "</tr>";
        }

        html += "</table>";
        html += "</div>";

    }

    if (mach.size() > 0) {
        html += "<div align=center><br/>";

        html += QString("<table class=curved style=\"page-break-before:auto;\" "+table_width+">");

        html += "<thead>";
        html += "<tr bgcolor='"+heading_color+"'><th colspan=7 align=center><font size=+2>" + tr("Machine Information") + "</font></th></tr>";

        html += QString("<tr><td><b>%1</b></td><td><b>%2</b></td><td><b>%3</b></td><td><b>%4</b></td><td><b>%5</b></td><td><b>%6</b></td><td><b>%7</b></td></tr>")
                .arg(STR_TR_Brand)
                .arg(STR_TR_Series)
                .arg(STR_TR_Model)
                .arg(STR_TR_Serial)
                .arg(tr("First Use"))
                .arg(tr("Last Use"))
                .arg(STR_TR_AHI);

        html += "</thead>";

        Machine *m;

        for (int i = 0; i < mach.size(); i++) {
            m = mach.at(i);

            if (m->type() == MT_JOURNAL) { continue; }

            QDate d1 = m->FirstDay();
            QDate d2 = m->LastDay();
            QString ahi;
            if (m->type() == MT_CPAP) {
                float a = calcAHI(d1,d2);
                ahi = QString::number(a,'f',2);
            }
            QString mn = m->modelnumber();
            html += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td><td>%7</td></tr>")
                    .arg(m->brand())
                    .arg(m->series())
                    .arg(m->model() +
                         (mn.isEmpty() ? "" : QString(" (") + mn + QString(")")))
                    .arg(m->serial())
                    .arg(d1.toString(Qt::SystemLocaleShortDate))
                    .arg(d2.toString(Qt::SystemLocaleShortDate))
                    .arg(ahi);

        }

        html += "</table>";
        html += "</div>";
    }

    html += "<script type='text/javascript' language='javascript' src='qrc:/docs/script.js'></script>";
    //updateFavourites();
    html += htmlFooter();
    return html;
}


QString StatisticsRow::value(QDate start, QDate end)
{
    const int decimals=2;
    QString value;
    float days = p_profile->countDays(type, start, end);

    // Handle special data sources first
    if (calc == SC_AHI) {
        value = QString("%1").arg(calcAHI(start, end), 0, 'f', decimals);
    } else if (calc == SC_HOURS) {
        value = QString("%1").arg(formatTime(p_profile->calcHours(type, start, end) / days));
    } else if (calc == SC_COMPLIANCE) {
        float c = p_profile->countCompliantDays(type, start, end);
        float p = (100.0 / days) * c;
        value = QString("%1%").arg(p, 0, 'f', 0);
    } else if (calc == SC_DAYS) {
        value = QString("%1").arg(p_profile->countDays(type, start, end));
    } else if ((calc == SC_COLUMNHEADERS) || (calc == SC_SUBHEADING) || (calc == SC_UNDEFINED))  {
    } else {
        //
        ChannelID code=channel();

        EventDataType val = 0;
        QString fmt = "%1";
        if (code != NoChannel) {
            switch(calc) {
            case SC_AVG:
                val = p_profile->calcAvg(code, type, start, end);
                break;
            case SC_WAVG:
                val = p_profile->calcWavg(code, type, start, end);
                break;
            case SC_MEDIAN:
                val = p_profile->calcPercentile(code, 0.5F, type, start, end);
                break;
            case SC_90P:
                val = p_profile->calcPercentile(code, 0.9F, type, start, end);
                break;
            case SC_MIN:
                val = p_profile->calcMin(code, type, start, end);
                break;
            case SC_MAX:
                val = p_profile->calcMax(code, type, start, end);
                break;
            case SC_CPH:
                val = p_profile->calcCount(code, type, start, end) / p_profile->calcHours(type, start, end);
                break;
            case SC_SPH:
                fmt += "%";
                val = 100.0 / p_profile->calcHours(type, start, end) * p_profile->calcSum(code, type, start, end) / 3600.0;
                break;
            case SC_ABOVE:
                fmt += "%";
                val = 100.0 / p_profile->calcHours(type, start, end) * (p_profile->calcAboveThreshold(code, schema::channel[code].upperThreshold(), type, start, end) / 60.0);
                break;
            case SC_BELOW:
                fmt += "%";
                val = 100.0 / p_profile->calcHours(type, start, end) * (p_profile->calcBelowThreshold(code, schema::channel[code].lowerThreshold(), type, start, end) / 60.0);
                break;
            default:
                break;
            };
        }

        if ((val == std::numeric_limits<EventDataType>::min()) || (val == std::numeric_limits<EventDataType>::max())) {
            value = "Err";
        } else {
            value = fmt.arg(val, 0, 'f', decimals);
        }
    }

    return value;
}
