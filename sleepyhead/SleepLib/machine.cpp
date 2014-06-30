/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * SleepLib Machine Class Implementation
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include <QApplication>
#include <QDir>
#include <QProgressBar>
#include <QDebug>
#include <QString>
#include <QObject>
#include <QThreadPool>

#include <time.h>

#include "machine.h"
#include "profiles.h"
#include <algorithm>
#include "SleepLib/schema.h"

extern QProgressBar *qprogress;

//////////////////////////////////////////////////////////////////////////////////////////
// Machine Base-Class implmementation
//////////////////////////////////////////////////////////////////////////////////////////
Machine::Machine(Profile *p, MachineID id)
{
    day.clear();
    highest_sessionid = 0;
    profile = p;

    if (!id) {
        srand(time(nullptr));
        MachineID temp;

        do {
            temp = rand();
        } while (profile->machlist.find(temp) != profile->machlist.end());

        m_id = temp;

    } else { m_id = id; }

    qDebug() << "Create Machine: " << hex << m_id; //%lx",m_id);
    m_type = MT_UNKNOWN;
    firstsession = true;
}
Machine::~Machine()
{
    qDebug() << "Destroy Machine" << m_class << hex << m_id;

    for (QMap<QDate, Day *>::iterator d = day.begin(); d != day.end(); d++) {
        delete d.value();
    }
}
Session *Machine::SessionExists(SessionID session)
{
    if (sessionlist.find(session) != sessionlist.end()) {
        return sessionlist[session];
    } else {
        return nullptr;
    }
}

// Find date this session belongs in
QDate Machine::pickDate(qint64 first)
{
    QTime split_time = PROFILE.session->daySplitTime();
    int combine_sessions = PROFILE.session->combineCloseSessions();

    QDateTime d2 = QDateTime::fromTime_t(first / 1000);

    QDate date = d2.date();
    QTime time = d2.time();

    int closest_session = 0;

    if (time < split_time) {
        date = date.addDays(-1);
    } else if (combine_sessions > 0) {
        QMap<QDate, Day *>::iterator dit = day.find(date.addDays(-1)); // Check Day Before

        if (dit != day.end()) {
            QDateTime lt = QDateTime::fromTime_t(dit.value()->last() / 1000L);
            closest_session = lt.secsTo(d2) / 60;

            if (closest_session < combine_sessions) {
                date = date.addDays(-1);
            }
        }
    }

    return date;
}

bool Machine::AddSession(Session *s, Profile *p)
{
    Q_ASSERT(s != nullptr);
    Q_ASSERT(p != nullptr);

    if (profile->session->ignoreOlderSessions()) {
        qint64 ignorebefore = profile->session->ignoreOlderSessionsDate().toMSecsSinceEpoch();
        if (s->last() < ignorebefore) {
            skipped_sessions++;
            return false;
        }
    }

    if (s->session() > highest_sessionid) {
        highest_sessionid = s->session();
    }

    QTime split_time = PROFILE.session->daySplitTime();
    int combine_sessions = PROFILE.session->combineCloseSessions();
    int ignore_sessions = PROFILE.session->ignoreShortSessions();

    // ResMed machines can't do this.. but don't really want to do a slow string compare here

    int session_length = s->last() - s->first();
    session_length /= 60000;

    sessionlist[s->session()] = s; // To make sure it get's saved later even if it's not wanted.

    //int drift=PROFILE.cpap->clockDrift();

    QDateTime d2 = QDateTime::fromTime_t(s->first() / 1000);

    QDate date = d2.date();
    QTime time = d2.time();

    QMap<QDate, Day *>::iterator dit, nextday;

    bool combine_next_day = false;
    int closest_session = 0;

    if (time < split_time) {
        date = date.addDays(-1);
    } else if (combine_sessions > 0) {
        dit = day.find(date.addDays(-1)); // Check Day Before

        if (dit != day.end()) {
            QDateTime lt = QDateTime::fromTime_t(dit.value()->last() / 1000);
            closest_session = lt.secsTo(d2) / 60;

            if (closest_session < combine_sessions) {
                date = date.addDays(-1);
            }
        } else {
            nextday = day.find(date.addDays(1)); // Check Day Afterwards

            if (nextday != day.end()) {
                QDateTime lt = QDateTime::fromTime_t(nextday.value()->first() / 1000);
                closest_session = d2.secsTo(lt) / 60;

                if (closest_session < combine_sessions) {
                    // add todays here. pull all tomorrows records to this date.
                    combine_next_day = true;
                }
            }
        }
    }

    if (session_length < ignore_sessions) {
        // keep the session to save importing it again, but don't add it to the day record this time
        return true;
    }

    if (!firstsession) {
        if (firstday > date) { firstday = date; }

        if (lastday < date) { lastday = date; }
    } else {
        firstday = lastday = date;
        firstsession = false;
    }


    Day *dd = nullptr;
    dit = day.find(date);

    if (dit == day.end()) {
        //QString dstr=date.toString("yyyyMMdd");
        //qDebug("Adding Profile Day %s",dstr.toLatin1().data());
        dd = new Day(this);
        day[date] = dd;
        // Add this Day record to profile
        p->AddDay(date, dd, m_type);
    } else {
        dd = *dit;
    }

    dd->AddSession(s);

    if (combine_next_day) {
        for (QList<Session *>::iterator i = nextday.value()->begin(); i != nextday.value()->end(); i++) {
            dd->AddSession(*i);
        }

        QMap<QDate, QList<Day *> >::iterator nd = p->daylist.find(date.addDays(1));

        for (QList<Day *>::iterator i = nd->begin(); i != nd->end(); i++) {
            if (*i == nextday.value()) {
                nd.value().erase(i);
            }
        }

        day.erase(nextday);
    }

    return true;
}

// This functions purpose is murder and mayhem... It deletes all of a machines data.
bool Machine::Purge(int secret)
{
    // Boring api key to stop this function getting called by accident :)
    if (secret != 3478216) { return false; }

    QString path = profile->Get(properties[STR_PROP_Path]);

    QDir dir(path);

    if (!dir.exists()) { // It doesn't exist anyway.
        return true;
    }

    if (!dir.isReadable()) {
        return false;
    }


    qDebug() << "Purging" << m_class << properties[STR_PROP_Serial] << dir.absoluteFilePath(path);

    // Create a copy of the list so the hash can be manipulated
    QList<Session *> sessions = sessionlist.values();

    // Clean up any loaded sessions from memory first..
    bool success = true;
    for (int i=0; i < sessions.size(); ++i) {
        Session * sess = sessions[i];
        if (!sess->Destroy()) {
            qDebug() << "Could not destroy "+ m_class+" ("+properties[STR_PROP_Serial]+") session" << sess->session();
            success = false;
        } else {
            sessionlist.erase(sessionlist.find(sess->session()));
        }
        delete sess;
    }

    // Clean up any straggling files (like from short sessions not being loaded...)
    dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    dir.setSorting(QDir::Name);

    QFileInfoList list = dir.entryInfoList();
    int could_not_kill = 0;

    int size = list.size();
    for (int i = 0; i < size; ++i) {
        QFileInfo fi = list.at(i);
        QString fullpath = fi.canonicalFilePath();

        QString ext_s = fullpath.section('.', -1);
        bool ok;
        ext_s.toInt(&ok, 10);

        if (ok) {
            qDebug() << "Deleting " << QDir::toNativeSeparators(fullpath);
            if (!dir.remove(fullpath)) {
                qDebug() << "Could not purge file" << fullpath;
                success=false;
                could_not_kill++;
            }
        } else {
            qDebug() << "Didn't bother deleting cruft file" << fullpath;
            // cruft file..
        }

    }

    if (could_not_kill > 0) {
        qWarning() << "Could not purge path" << could_not_kill << "files in " << path;
        return false;
    }

    return true;
}

//const quint32 channel_version=1;


bool Machine::Load()
{
    QString path = profile->Get(
                       properties[STR_PROP_Path]); //STR_GEN_DataFolder)+"/"+m_class+"_"+hexid();

    QDir dir(path);
    qDebug() << "Loading " << QDir::toNativeSeparators(path);

    if (!dir.exists() || !dir.isReadable()) {
        return false;
    }

    dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    dir.setSorting(QDir::Name);

    QFileInfoList list = dir.entryInfoList();

    typedef QVector<QString> StringList;
    QMap<SessionID, StringList> sessfiles;
    QMap<SessionID, StringList>::iterator s;

    QString fullpath, ext_s, sesstr;
    int ext;
    SessionID sessid;
    bool ok;

    for (int i = 0; i < list.size(); i++) {
        QFileInfo fi = list.at(i);
        fullpath = fi.canonicalFilePath();
        ext_s = fi.fileName().section(".", -1);
        ext = ext_s.toInt(&ok, 10);

        if (!ok) { continue; }

        sesstr = fi.fileName().section(".", 0, -2);
        sessid = sesstr.toLong(&ok, 16);

        if (!ok) { continue; }

        if (sessfiles[sessid].capacity() == 0) { sessfiles[sessid].resize(3); }

        sessfiles[sessid][ext] = fi.canonicalFilePath();
    }

    int size = sessfiles.size();
    int cnt = 0;

    for (s = sessfiles.begin(); s != sessfiles.end(); s++) {
        if ((++cnt % 50) == 0) { // This is slow.. :-/
            if (qprogress) { qprogress->setValue((float(cnt) / float(size) * 100.0)); }

            QApplication::processEvents();
        }

        Session *sess = new Session(this, s.key());

        if (sess->LoadSummary(s.value()[0])) {
            sess->SetEventFile(s.value()[1]);
            //sess->OpenEvents();
            AddSession(sess, profile);
        } else {
            qWarning() << "Error unpacking summary data";
            delete sess;
        }
    }

    if (qprogress) { qprogress->setValue(100); }

    return true;
}
bool Machine::SaveSession(Session *sess)
{
    QString path = profile->Get(
                       properties[STR_PROP_Path]); //STR_GEN_DataFolder)+"/"+m_class+"_"+hexid();

    if (sess->IsChanged()) { sess->Store(path); }

    return true;
}

void Machine::queSaveList(Session * sess)
{
    if (!m_save_threads_running) {
        // Threads aren't being used.. so run the actual immediately...
        int i = (float(m_donetasks) / float(m_totaltasks) * 100.0);
        qprogress->setValue(i);
        QApplication::processEvents();

        sess->UpdateSummaries();
        sess->Store(profile->Get(properties[STR_PROP_Path]));

        if (!PROFILE.session->cacheSessions()) {
            sess->TrashEvents();
        }

    } else {
        listMutex.lock();
        m_savelist.append(sess);
        listMutex.unlock();
    }
}

Session *Machine::popSaveList()
{
    Session *sess = nullptr;
    listMutex.lock();

    if (!m_savelist.isEmpty()) {
        sess = m_savelist.at(0);
        m_savelist.pop_front();
        m_donetasks++;
    }

    listMutex.unlock();
    return sess;
}

// Call any time queing starts
void Machine::StartSaveThreads()
{
    m_savelist.clear();
    if (!p_profile->session->multithreading()) return;

    QString path = profile->Get(properties[STR_PROP_Path]);

    int threads = QThread::idealThreadCount();
    savelistSem = new QSemaphore(threads);
    savelistSem->acquire(threads);

    m_save_threads_running = true;
    m_donetasks=0;
    m_totaltasks=0;

    for (int i = 0; i < threads; i++) {
        SaveThread * thr = new SaveThread(this, path);
        QObject::connect(thr, SIGNAL(UpdateProgress(int)), qprogress, SLOT(setValue(int)));
        thread.push_back(thr);
        thread[i]->start();
    }

}

// Call when all queing is completed
void Machine::FinishSaveThreads()
{
    if (!m_save_threads_running)
        return;

    m_save_threads_running = false;

    // Wait for all tasks to finish
    while (!savelistSem->tryAcquire(thread.size(), 250)) {
        if (qprogress) {
            QApplication::processEvents();
        }
    }

    for (int i = 0; i < thread.size(); ++i) {
        while (thread[i]->isRunning()) {
            SaveThread::msleep(250);
            QApplication::processEvents();
        }
        QObject::disconnect(thread[i], SIGNAL(UpdateProgress(int)), qprogress, SLOT(setValue(int)));

        delete thread[i];
    }

    delete savelistSem;
}

void SaveThread::run()
{
    bool running = true;
    while (running) {
        Session *sess = machine->popSaveList();
        if (sess) {
            if (machine->m_donetasks % 10 == 0) {
                int i = (float(machine->m_donetasks) / float(machine->m_totaltasks) * 100.0);
                emit UpdateProgress(i);
            }
            sess->UpdateSummaries();
            machine->saveMutex.lock();
            sess->Store(path);
            machine->saveMutex.unlock();

            sess->TrashEvents();
        } else {
            if (!machine->m_save_threads_running) {
                break; // done
            } else {
                yieldCurrentThread(); // go do something else for a while
            }
        }
    }

    machine->savelistSem->release(1);
}



class SaveTask:public ImportTask
{
public:
    SaveTask(Session * s, Machine * m): sess(s), mach(m) {}
    virtual ~SaveTask() {}
    virtual void run();

protected:
    Session * sess;
    Machine * mach;
};

void SaveTask::run()
{
    sess->UpdateSummaries();
    mach->saveMutex.lock();
    sess->Store(p_profile->Get(mach->properties[STR_PROP_Path]));
    mach->saveMutex.unlock();
    sess->TrashEvents();
}

void Machine::queTask(ImportTask * task)
{
    if (0) { //PROFILE.session->multithreading()) {
        m_tasklist.push_back(task);
        return;
    }

    task->run();
    return;
}

void Machine::runTasks()
{
    if (0) { //!PROFILE.session->multithreading()) {
        Q_ASSERT(m_tasklist.isEmpty());
        return;
    }
    QThreadPool * threadpool = QThreadPool::globalInstance();
    int m_totaltasks=m_tasklist.size();
    int m_currenttask=0;
    while (!m_tasklist.isEmpty()) {
        if (threadpool->tryStart(m_tasklist.at(0))) {
            m_tasklist.pop_front();
            float f = float(m_currenttask) / float(m_totaltasks) * 100.0;
            qprogress->setValue(f);
            m_currenttask++;
        }
        QApplication::processEvents();
    }
    QThreadPool::globalInstance()->waitForDone(-1);
}

bool Machine::Save()
{
    //int size;
    int cnt = 0;

    QString path = profile->Get(properties[STR_PROP_Path]);
    QDir dir(path);

    if (!dir.exists()) {
        dir.mkdir(path);
    }

    QHash<SessionID, Session *>::iterator s;

    m_savelist.clear();

    for (s = sessionlist.begin(); s != sessionlist.end(); s++) {
        cnt++;

        if ((*s)->IsChanged()) {
            queTask(new SaveTask(*s, this));
        }
    }

    runTasks();
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////
// CPAP implmementation
//////////////////////////////////////////////////////////////////////////////////////////
CPAP::CPAP(Profile *p, MachineID id): Machine(p, id)
{
    m_type = MT_CPAP;
}

CPAP::~CPAP()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// Oximeter Class implmementation
//////////////////////////////////////////////////////////////////////////////////////////
Oximeter::Oximeter(Profile *p, MachineID id): Machine(p, id)
{
    m_type = MT_OXIMETER;
}

Oximeter::~Oximeter()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// SleepStage Class implmementation
//////////////////////////////////////////////////////////////////////////////////////////
SleepStage::SleepStage(Profile *p, MachineID id): Machine(p, id)
{
    m_type = MT_SLEEPSTAGE;
}
SleepStage::~SleepStage()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// PositionSensor Class implmementation
//////////////////////////////////////////////////////////////////////////////////////////
PositionSensor::PositionSensor(Profile *p, MachineID id): Machine(p, id)
{
    m_type = MT_POSITION;
}
PositionSensor::~PositionSensor()
{
}


ChannelID NoChannel, SESSION_ENABLED, CPAP_SummaryOnly;
ChannelID CPAP_IPAP, CPAP_IPAPLo, CPAP_IPAPHi, CPAP_EPAP, CPAP_EPAPLo, CPAP_EPAPHi, CPAP_Pressure,
          CPAP_PS, CPAP_Mode, CPAP_AHI,
          CPAP_PressureMin, CPAP_PressureMax, CPAP_RampTime, CPAP_RampPressure, CPAP_Obstructive,
          CPAP_Hypopnea,
          CPAP_ClearAirway, CPAP_Apnea, CPAP_CSR, CPAP_LeakFlag, CPAP_ExP, CPAP_NRI, CPAP_VSnore,
          CPAP_VSnore2,
          CPAP_RERA, CPAP_PressurePulse, CPAP_FlowLimit, CPAP_SensAwake, CPAP_FlowRate, CPAP_MaskPressure,
          CPAP_MaskPressureHi,
          CPAP_RespEvent, CPAP_Snore, CPAP_MinuteVent, CPAP_RespRate, CPAP_TidalVolume, CPAP_PTB, CPAP_Leak,
          CPAP_LeakMedian, CPAP_LeakTotal, CPAP_MaxLeak, CPAP_FLG, CPAP_IE, CPAP_Te, CPAP_Ti, CPAP_TgMV,
          CPAP_UserFlag1, CPAP_UserFlag2, CPAP_UserFlag3, CPAP_BrokenSummary, CPAP_BrokenWaveform, CPAP_RDI,
          CPAP_PresReliefSet, CPAP_PresReliefMode, CPAP_PresReliefType, CPAP_PSMin, CPAP_PSMax, CPAP_Test1,
          CPAP_Test2;


ChannelID RMS9_E01, RMS9_E02, RMS9_EPR, RMS9_EPRSet, RMS9_SetPressure, RMS9_MaskOnTime;
ChannelID INTP_SmartFlex;
ChannelID INTELLIPAP_Unknown1, INTELLIPAP_Unknown2;

ChannelID PRS1_00, PRS1_01, PRS1_08, PRS1_0A, PRS1_0B, PRS1_0C, PRS1_0E, PRS1_0F, CPAP_LargeLeak, PRS1_12,
          PRS1_FlexMode, PRS1_FlexSet, PRS1_HumidStatus, CPAP_HumidSetting, PRS1_SysLock,
          PRS1_SysOneResistStat,
          PRS1_SysOneResistSet, PRS1_HoseDiam, PRS1_AutoOn, PRS1_AutoOff, PRS1_MaskAlert, PRS1_ShowAHI;

ChannelID OXI_Pulse, OXI_SPO2, OXI_PulseChange, OXI_SPO2Drop, OXI_Plethy;

ChannelID Journal_Notes, Journal_Weight, Journal_BMI, Journal_ZombieMeter, Bookmark_Start,
          Bookmark_End, Bookmark_Notes;


ChannelID ZEO_SleepStage, ZEO_ZQ, ZEO_TotalZ, ZEO_TimeToZ, ZEO_TimeInWake, ZEO_TimeInREM,
          ZEO_TimeInLight, ZEO_TimeInDeep, ZEO_Awakenings,
          ZEO_AlarmReason, ZEO_SnoozeTime, ZEO_WakeTone, ZEO_WakeWindow, ZEO_AlarmType, ZEO_MorningFeel,
          ZEO_FirmwareVersion,
          ZEO_FirstAlarmRing, ZEO_LastAlarmRing, ZEO_FirstSnoozeTime, ZEO_LastSnoozeTime, ZEO_SetAlarmTime,
          ZEO_RiseTime;

ChannelID POS_Orientation, POS_Inclination;
