/*
 SleepyHead Preferences Dialog GUI Headers
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QModelIndex>
#include <QListWidgetItem>
#include <QStringListModel>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include "SleepLib/profiles.h"

namespace Ui {
    class PreferencesDialog;
}


/*! \class MySortFilterProxyModel
    \brief Enables the Graph tabs view to be filtered
    */
class MySortFilterProxyModel:public QSortFilterProxyModel
{
    Q_OBJECT
public:
    MySortFilterProxyModel(QObject *parent = 0);

protected:
    //! \brief Simply extends filterAcceptRow to scan children as well
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;

};

/*! \struct MaskProfile
    \brief This in still a work in progress, and may be used in Unintentional leaks calculations.
    */
struct MaskProfile {
    QString name;
    EventDataType pflow[5][2];
};

/*! \class PreferencesDialog
    \brief SleepyHead's Main Preferences Window

    This provides the Preferences form and logic to alter Preferences for SleepyHead
*/
class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent, Profile * _profile);
    ~PreferencesDialog();

    //! \brief Save the current preferences, called when Ok button is clicked on
    void Save();

    //! \brief Updates the date text of the last time updates where checked
    void RefreshLastChecked();

private slots:
    void on_eventTable_doubleClicked(const QModelIndex &index);
    void on_combineSlider_valueChanged(int value);

    void on_IgnoreSlider_valueChanged(int value);

    void on_checkForUpdatesButton_clicked();

    void on_addImportLocation_clicked();

    void on_removeImportLocation_clicked();

    void on_graphView_activated(const QModelIndex &index);

    void on_graphFilter_textChanged(const QString &arg1);

    void graphModel_changed(QStandardItem * item);

    void on_resetGraphButton_clicked();

    //void on_genOpWidget_itemActivated(QListWidgetItem *item);

    void on_maskTypeCombo_activated(int index);

private:
    //! \brief Populates the Graph Model view with data from the Daily, Overview & Oximetry gGraphView objects
    void resetGraphModel();

    Ui::PreferencesDialog *ui;
    Profile * profile;
    QHash<int,QColor> m_new_colors;
    QStringList importLocations;
    QStringListModel *importModel;
    MySortFilterProxyModel *graphFilterModel;
    QStandardItemModel *graphModel;
    QHash<QString,Preference> general;
};


#endif // PREFERENCESDIALOG_H
