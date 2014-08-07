/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * Daily GUI Headers
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef DAILY_H
#define DAILY_H


#include <QMenu>
#include <QAction>
#include <QWidget>
#include <QTreeWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QtOpenGL/QGLContext>
#include <QScrollBar>
#include <QTableWidgetItem>
#include <QWebView>
#include <QWebPage>
#include "Graphs/gSummaryChart.h"

#include <SleepLib/profiles.h>
#include "mainwindow.h"
#include "Graphs/gGraphView.h"

#include "Graphs/gLineChart.h"
#include "sessionbar.h"

namespace Ui {
    class Daily;
}

class MyWebPage:public QWebPage
{
Q_OBJECT
public:
    MyWebPage(QObject *parent = 0);
protected:
    QObject *createPlugin(const QString & classid, const QUrl & url, const QStringList & paramNames, const QStringList & paramValues);
};

class MyWebView:public QWebView
{
   Q_OBJECT
   private:
      MyWebPage m_page;
   public:
      MyWebView(QWidget *parent = 0);
};



class MainWindow;

/*! \class Daily
    \brief SleepyHead's Daily view which displays the calendar and all the graphs relative to a selected Day
    */
class Daily : public QWidget
{
    Q_OBJECT

public:
    /*! \fn Daily()
        \brief Constructs a Daily object
        \param parent * (QObject parent)
        \param shared *

        Creates all the graph objects and adds them to the main gGraphView area for this tab.

        shared is not used for daily object, as it contains the default Context..
        */
    explicit Daily(QWidget *parent, gGraphView *shared);
    ~Daily();

    /*! \fn ReloadGraphs()
        \brief Reload all graph information from disk and updates the view.
        */
    void ReloadGraphs();
    /*! \fn ResetGraphLayout()
        \brief Resets all graphs in the main gGraphView back to constant heights.
        */
    void ResetGraphLayout();

    /*! \fn graphView()
        \returns the main graphView area for the Daily View
        */
    gGraphView *graphView() { return GraphView; }

    /*! \fn RedrawGraphs()
        \brief Calls updateGL on the main graphView area, redrawing the OpenGL area
        */
    void RedrawGraphs();

    /*! \fn LoadDate()
        \brief Selects a new day object, unloading the previous one, and loads the graph data for the supplied date.
        \param QDate date
        */
    void LoadDate(QDate date);

    /*! \fn getDate()
        \brief Returns the most recently loaded Date
        \return QDate
        */
    QDate getDate() { return previous_date; }

    /*! \fn UnitsChanged()
        \brief Called by Profile editor when measurement units are changed
        */
    void UnitsChanged();

    /*! \fn GetJournalSession(QDate date)
        \brief Looks up if there is a journal object for a supplied date
        \param QDate date
        \returns Session * containing valid Journal Session object or nullptr if none found.
    */
    Session * GetJournalSession(QDate date);

    QString GetDetailsText();
    /*! \fn eventBreakdownPie()
        \brief Returns a pointer to the Event Breakdown Piechart for the Report Printing module
        \returns gGraph * object containing this chart
        */
    gGraph * eventBreakdownPie() { return graphlist["EventBreakdown"]; }
    QWidget * sessionBar() { return sessbar; }

    void clearLastDay();

    /*! \fn Unload(QDate date)
        \brief Saves any journal changes for the provided date.
        \param QDate date
        */
    void Unload(QDate date=QDate());

    void setSidebarVisible(bool visible);
    void setCalendarVisible(bool visible);

    void addBookmark(qint64 st, qint64 et, QString text);

private slots:

    /*! \fn on_calendar_currentPageChanged(int year, int month);
        \brief Scans through all days for this month, updating the day colors for the calendar object
        \param int year
        \param int month
        */
    void on_calendar_currentPageChanged(int year, int month);

    /*! \fn on_calendar_selectionChanged();
        \brief Called when the calendar object is clicked. Selects and loads a new date, unloading the previous one.
        */
    void on_calendar_selectionChanged();

    /* Journal Notes edit buttons I don't want to document */
    void on_JournalNotesItalic_clicked();
    void on_JournalNotesBold_clicked();
    void on_JournalNotesFontsize_activated(int index);
    void on_JournalNotesColour_clicked();
    void on_JournalNotesUnderline_clicked();

    void on_treeWidget_itemSelectionChanged();


    /*! \fn on_nextDayButton_clicked();
        \brief Step backwards one day (if possible)
        */
    void on_prevDayButton_clicked();

    /*! \fn on_nextDayButton_clicked();
        \brief Step forward one day (if possible)
        */
    void on_nextDayButton_clicked();

    /*! \fn on_calButton_toggled();
        \brief Hides the calendar and put it out of the way, giving more room for the Details area.
        */
    void on_calButton_toggled(bool checked);

    /*! \fn on_todayButton_clicked();
        \brief Select the most recent day.
        */
    void on_todayButton_clicked();

    /*! \fn Link_clicked(const QUrl &url);
        \brief Called when a link is clicked on in the HTML Details tab
        \param const QUrl & url
        */
    void Link_clicked(const QUrl &url);

    void on_evViewSlider_valueChanged(int value);

    /*! \fn on_treeWidget_itemClicked(QTreeWidgetItem *item, int column);
        \brief Called when an event is selected in the Event tab.. Zooms into the graph area.
        \param QTreeWidgetItem *item
        \param int column
        */
    void on_treeWidget_itemClicked(QTreeWidgetItem *item, int column);

    /*! \fn graphtogglebutton_toggled(bool)
        \brief Called to hide/show a graph when on of the toggle bottoms underneath the graph area is clicked
        \param bool button status
        */
    void graphtogglebutton_toggled(bool);

    /*! \fn on_addBookmarkButton_clicked()
        \brief Current selected graph Area is added to Bookmark's list for this day's journal object.
        */
    void on_addBookmarkButton_clicked();

    /*! \fn on_removeBookmarkButton_clicked()
        \brief Currently selected bookmark is removed from this day's Bookmark list.
        */
    void on_removeBookmarkButton_clicked();

    /*! \fn on_bookmarkTable_itemClicked(QTableWidgetItem *item);
        \brief Called when a bookmark has been selected.. Zooms in on the area
        \param QTableWidgetItem *item
        */
    void on_bookmarkTable_itemClicked(QTableWidgetItem *item);

    /*! \fn on_bookmarkTable_itemChanged(QTableWidgetItem *item);
        \brief Called when bookmarks have been altered.. Saves the bookmark list to Journal object.
        */
    void on_bookmarkTable_itemChanged(QTableWidgetItem *item);


    /*! \fn on_ouncesSpinBox_valueChanged(int arg1);
        \brief Called when the zombie slider has been moved.. Updates the BMI dislpay and journal objects.

        Also Refreshes the Overview charts
        */
    void on_ZombieMeter_valueChanged(int value);

    void on_graphCombo_activated(int index);

    void on_toggleGraphs_clicked(bool checked);

    void on_zoomFullyOut_clicked();

    void on_resetLayoutButton_clicked();

    /*! \fn on_weightSpinBox_editingFinished();
        \brief Called when weight has changed.. Updates the BMI dislpay and journal objects.

        Also Refreshes the Overview charts
        */

    void on_weightSpinBox_editingFinished();

    /*! \fn on_ouncesSpinBox_editingFinished();
        \brief Called when weights ounces component has changed.. Updates the BMI dislpay and journal objects.

        Also Refreshes the Overview charts
        */
    void on_ouncesSpinBox_editingFinished();

    void on_ouncesSpinBox_valueChanged(int arg1);

    void on_weightSpinBox_valueChanged(double arg1);

    void doToggleSession(Session *);

    void on_eventsCombo_activated(int index);

    void on_toggleEvents_clicked(bool checked);

protected:
    virtual void closeEvent(QCloseEvent *);
    virtual void showEvent(QShowEvent *);

private:
    /*! \fn CreateJournalSession()
        \brief Create a new journal session for this date, if one doesn't exist.
        \param QDate date

        Creates a new journal Machine record if necessary.
        */
    Session * CreateJournalSession(QDate date);

    /*! \fn update_Bookmarks()
        \brief Saves the current bookmark list to the Journal object
        */
    void update_Bookmarks();

    /*! \fn Load(QDate date)
        \brief Selects a new day object, loads it's content and generates the HTML for the Details tab
        \param QDate date
        */
    void Load(QDate date);
    /*! \fn UpdateCalendarDay(QDate date)
        \brief Updates the calendar visual information, changing a dates color depending on what data is available.
        \param QDate date
        */
    void UpdateCalendarDay(QDate date);
    /*! \fn UpdateEventsTree(QDate date)
        \brief Populates the Events tree from the supplied Day object.
        \param QTreeWidget * tree
        \param Day *
        */
    void UpdateEventsTree(QTreeWidget * tree,Day *day);

    void updateCube();
    void updateGraphCombo();

    QString getSessionInformation(Day *cpap, Day *oxi, Day *stage, Day *posit);
    QString getMachineSettings(Day *cpap);
    QString getStatisticsInfo(Day *cpap, Day *oxi, Day *pos);
    QString getCPAPInformation(Day *cpap);
    QString getOximeterInformation(Day *oxi);
    QString getEventBreakdown(Day *cpap);
    QString getSleepTime(Day *cpap, Day *oxi);

    QHash<QString, gGraph *> graphlist;

    QList<Layer *> OXIData;
    QList<Layer *> CPAPData;
    QList<Layer *> STAGEData;
    QList<Layer *> POSData;
    QHash<QString,QPushButton *> GraphToggles;
    QVector<QAction *> GraphAction;
    QGLContext *offscreen_context;

    QList<int> splitter_sizes;
    Layer * AddCPAP(Layer *d) { CPAPData.push_back(d); return d; }
    Layer * AddSTAGE(Layer *d) { STAGEData.push_back(d); return d; }
    Layer * AddPOS(Layer *d) { POSData.push_back(d); return d; }
    Layer * AddOXI(Layer *d) { OXIData.push_back(d); return d; }

    void UpdateCPAPGraphs(Day *day);
    void UpdateOXIGraphs(Day *day);
    void UpdateSTAGEGraphs(Day *day);
    void UpdatePOSGraphs(Day *day);

    Ui::Daily *ui;
    QDate previous_date;
    QMenu *show_graph_menu;

    gGraphView *GraphView,*snapGV;
    MyScrollBar *scrollbar;
    QHBoxLayout *layout;
    QLabel *emptyToggleArea;
    QIcon * icon_on;
    QIcon * icon_off;

    SessionBar * sessbar;
    QLabel * dateDisplay;

    MyWebView * webView;
    Day * lastcpapday;

    gLineChart *leakchart;

    bool ZombieMeterMoved;
    bool BookmarksChanged;
};

#endif // DAILY_H
