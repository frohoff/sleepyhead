/* SleepLib PRS1 Loader Header
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
    PRS1(MachineID id = 0);
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
    QByteArray m_headerblock;

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

class PRS1Loader;


class PRS1Import:public ImportTask
{
public:
    PRS1Import(PRS1Loader * l, SessionID s, Machine * m): loader(l), sessionid(s), mach(m) {
        summary = nullptr;
        compliance = nullptr;
        event = nullptr;
        session = nullptr;
    }
    virtual ~PRS1Import() {
        delete compliance;
        delete summary;
        delete event;
        for (int i=0;i < waveforms.size(); ++i) {delete waveforms.at(i); }
    }

    virtual void run();

    PRS1DataChunk * compliance;
    PRS1DataChunk * summary;
    PRS1DataChunk * event;
    QList<PRS1DataChunk *> waveforms;
    QList<PRS1DataChunk *> oximetery;

    QString wavefile;
    QString oxifile;

    bool ParseCompliance();
    bool ParseSummary();
    bool ParseEvents();
    bool ParseWaveforms();
    bool ParseOximetery();


    bool ParseSummaryF0();
    bool ParseSummaryF0V4();
    bool ParseSummaryF3();
    bool ParseSummaryF5V0();
    bool ParseSummaryF5V1();
    bool ParseSummaryF6();


    //! \brief Parse a single data chunk from a .002 file containing event data for a standard system one machine
    bool ParseF0Events();

    //! \brief Parse a single data chunk from a .002 file containing event data for a family 5 ASV machine (which has a different format)
    bool ParseF5Events();

protected:
    Session * session;
    PRS1Loader * loader;
    SessionID sessionid;
    Machine * mach;

    int summary_duration;
};

/*! \class PRS1Loader
    \brief Philips Respironics System One Loader Module
    */
class PRS1Loader : public CPAPLoader
{
  public:
    PRS1Loader();
    virtual ~PRS1Loader();

    QString checkDir(const QString & path);
    bool PeekProperties(MachineInfo & info, QString path, Machine * mach = nullptr);


    //! \brief Detect if the given path contains a valid Folder structure
    virtual bool Detect(const QString & path);

    virtual MachineInfo PeekInfo(const QString & path);

    //! \brief Scans directory path for valid PRS1 signature
    virtual int Open(QString path);

    //! \brief Returns the database version of this loader
    virtual int Version() { return prs1_data_version; }

    //! \brief Return the loaderName, in this case "PRS1"
    virtual const QString &loaderName() { return prs1_class_name; }

    QList<PRS1DataChunk *> ParseFile(QString path);

    //! \brief Register this Module to the list of Loaders, so it knows to search for PRS1 data.
    static void Register();

    virtual MachineInfo newInfo() {
        return MachineInfo(MT_CPAP, 0, prs1_class_name, QObject::tr("Philips Respironics"), QString(), QString(), QString(), QObject::tr("System One"), QDateTime::currentDateTime(), prs1_data_version);
    }


    virtual QString PresReliefLabel() { return QObject::tr(""); }
    virtual ChannelID PresReliefMode() { return PRS1_FlexMode; }
    virtual ChannelID PresReliefLevel() { return PRS1_FlexLevel; }

    virtual ChannelID HumidifierConnected() { return PRS1_HumidStatus; }
    virtual ChannelID HumidifierLevel() { return PRS1_HumidLevel; }

    void initChannels();


    QHash<SessionID, PRS1Import*> sesstasks;

  protected:
    QString last;
    QHash<QString, Machine *> PRS1List;

    //! \brief Opens the SD folder structure for this machine, scans for data files and imports any new sessions
    int OpenMachine(QString path);

//    //! \brief Parses "properties.txt" file containing machine information
//    bool ParseProperties(Machine *m, QString filename);

    //! \brief Parse a .005 waveform file, extracting Flow Rate waveform (and Mask Pressure data if available)
    bool OpenWaveforms(SessionID sid, QString filename);

    //! \brief Parse a data chunk from the .000 (brick) and .001 (summary) files.
    bool ParseSummary(Machine *mach, qint32 sequence, quint32 timestamp, unsigned char *data,
                      quint16 size, int family, int familyVersion);



    //! \brief Open a PRS1 data file, and break into data chunks, delivering them to the correct parser.
    bool OpenFile(Machine *mach, QString filename);

    QHash<SessionID, Session *> extra_session;

    //! \brief PRS1 Data files can store multiple sessions, so store them in this list for later processing.
    QHash<SessionID, Session *> new_sessions;

    qint32 summary_duration;
};

#endif // PRS1LOADER_H
