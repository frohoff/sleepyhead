/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * SleepLib PRS1 Loader Header
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef PRS1LOADER_H
#define PRS1LOADER_H
//#include <map>
//using namespace std;
#include "SleepLib/machine.h" // Base class: MachineLoader
#include "SleepLib/machine_loader.h"
#include "SleepLib/profiles.h"


//********************************************************************************************
/// IMPORTANT!!!
//********************************************************************************************
// Please INCREMENT the following value when making changes to this loaders implementation.
//
const int prs1_data_version = 13;
//
//********************************************************************************************

/*! \class PRS1
    \brief PRS1 customized machine object (via CPAP)
    */
class PRS1: public CPAP
{
  public:
    PRS1(Profile *p, MachineID id = 0);
    virtual ~PRS1();
};


const int max_load_buffer_size = 1024 * 1024;
const QString prs1_class_name = STR_MACH_PRS1;

/*! \struct PRS1Waveform
    \brief Used in PRS1 Waveform Parsing */
struct PRS1Waveform {
    PRS1Waveform(quint16 i, quint8 f) {
        interleave = i;
        sample_format = f;
    }
    quint16 interleave;
    quint8 sample_format;
};


class PRS1DataChunk
{
    friend class PRS1DataGroup;
public:
    PRS1DataChunk() {
        timestamp = 0;
        ext = 255;
        sessionid = 0;
        htype = 0;
        family = 0;
        familyVersion = 0;
        duration = 0;

    }
    ~PRS1DataChunk() {
    }
    inline int size() const { return m_data.size(); }

    QByteArray m_data;

    SessionID sessionid;

    quint8 fileVersion;
    quint8 ext;
    quint8 htype;
    quint8 family;
    quint8 familyVersion;
    quint32 timestamp;

    quint16 duration;

    QList<PRS1Waveform> waveformInfo;
};

class PRS1SessionData
{
public:
    PRS1SessionData() {
        compliance = summary = event = nullptr;
        session = nullptr;
    }
    PRS1SessionData(const PRS1SessionData & copy) {
        session = copy.session;
        compliance = copy.compliance;
        summary = copy.summary;
        event = copy.event;
        waveforms = copy.waveforms;
    }
    ~PRS1SessionData() {
        delete compliance;
        delete summary;
        delete event;
        Q_FOREACH(PRS1DataChunk * c, waveforms) {
            delete c;
        }
    }

    bool ParseCompliance();
    bool ParseSummary();
    bool ParseEvents();
    bool ParseWaveforms();

    //! \brief Parse a single data chunk from a .002 file containing event data for a standard system one machine
    bool ParseF0Events();

    //! \brief Parse a single data chunk from a .002 file containing event data for a family 5 ASV machine (which has a different format)
    bool ParseF5Events();

    Session * session;

    PRS1DataChunk * compliance;
    PRS1DataChunk * summary;
    PRS1DataChunk * event;
    QList<PRS1DataChunk *> waveforms;
};

class PRS1Loader;

struct PRS1FileGroup
{
    PRS1FileGroup() { loader = NULL; }
    PRS1FileGroup(const PRS1FileGroup & copy) {
        compliance = copy.compliance;
        summary = copy.summary;
        event = copy.event;
        waveform = copy.waveform;
        loader = copy.loader;
    }
    ~PRS1FileGroup() {
    }

    QString compliance;
    QString summary;
    QString event;
    QString waveform;

    bool ParseFile(QString path);
    void ParseChunks(PRS1Loader *);

    PRS1Loader * loader;

    QMap<SessionID, PRS1SessionData*> sessions;
};

class PRS1Import:public ImportTask
{
public:
    PRS1Import(PRS1Loader * l, SessionID s, PRS1FileGroup *g, Machine * m): loader(l), sessionid(s), group(g), mach(m) {}
    virtual ~PRS1Import() {}
    virtual void run();

protected:
    PRS1Loader * loader;
    SessionID sessionid;
    PRS1FileGroup *group;
    Machine * mach;
};

/*! \class PRS1Loader
    \brief Philips Respironics System One Loader Module
    */
class PRS1Loader : public MachineLoader
{
  public:
    PRS1Loader();
    virtual ~PRS1Loader();

    //! \brief Detect if the given path contains a valid Folder structure
    virtual bool Detect(const QString & path);

    //! \brief Scans directory path for valid PRS1 signature
    virtual int Open(QString path, Profile *profile);

    //! \brief Returns the database version of this loader
    virtual int Version() { return prs1_data_version; }

    //! \brief Return the ClassName, in this case "PRS1"
    virtual const QString &ClassName() { return prs1_class_name; }

    //! \brief Create a new PRS1 machine record, indexed by Serial number.
    Machine *CreateMachine(QString serial, Profile *profile);

    //! \brief Register this Module to the list of Loaders, so it knows to search for PRS1 data.
    static void Register();

    QHash<SessionID, PRS1FileGroup*> prs1sessions;

  protected:
    QString last;
    QHash<QString, Machine *> PRS1List;

    //! \brief Opens the SD folder structure for this machine, scans for data files and imports any new sessions
    int OpenMachine(Machine *m, QString path, Profile *profile);

    //! \brief Parses "properties.txt" file containing machine information
    bool ParseProperties(Machine *m, QString filename);

    //bool OpenSummary(Session *session,QString filename);
    //bool OpenEvents(Session *session,QString filename);

    //! \brief Parse a .005 waveform file, extracting Flow Rate waveform (and Mask Pressure data if available)
    bool OpenWaveforms(SessionID sid, QString filename);

    // //! \brief ParseWaveform chunk.. Currently unused, as the old one works fine.
    //bool ParseWaveform(qint32 sequence, quint32 timestamp, unsigned char *data, quint16 size, quint16 duration, quint16 num_signals, quint16 interleave, quint8 sample_format);

    //! \brief Parse a data chunk from the .000 (brick) and .001 (summary) files.
    bool ParseSummary(Machine *mach, qint32 sequence, quint32 timestamp, unsigned char *data,
                      quint16 size, int family, int familyVersion);



    //! \brief Open a PRS1 data file, and break into data chunks, delivering them to the correct parser.
    bool OpenFile(Machine *mach, QString filename);

    //bool Parse002(Session *session,unsigned char *buffer,int size,qint64 timestamp,long fpos);
    //bool Parse002ASV(Session *session,unsigned char *buffer,int size,qint64 timestamp,long fpos);
    unsigned char *m_buffer;
    QHash<SessionID, Session *> extra_session;

    //! \brief PRS1 Data files can store multiple sessions, so store them in this list for later processing.
    QHash<SessionID, Session *> new_sessions;

    qint32 summary_duration;
};

#endif // PRS1LOADER_H
