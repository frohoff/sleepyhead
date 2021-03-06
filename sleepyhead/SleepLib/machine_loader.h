/* SleepLib MachineLoader Base Class Header
 *
 * Copyright (c) 2011 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef MACHINE_LOADER_H
#define MACHINE_LOADER_H

#include <QMutex>
#include <QRunnable>
#include <QPixmap>


#include "profiles.h"
#include "machine.h"
#include "zlib.h"


class MachineLoader;
enum DeviceStatus { NEUTRAL, IMPORTING, LIVE, DETECTING };

const QString genericPixmapPath = ":/icons/mask.png";


/*! \class MachineLoader
    \brief Base class to derive a new Machine importer from
    */
class MachineLoader: public QObject
{
    Q_OBJECT
    friend class ImportThread;
    friend class Machine;
  public:
    MachineLoader();
    virtual ~MachineLoader();

    //! \brief Detect if the given path contains a valid folder structure
    virtual bool Detect(const QString & path) = 0;

    //! \brief Look up and return machine model information stored at path
    virtual MachineInfo PeekInfo(const QString & path) { Q_UNUSED(path); return MachineInfo(); }

    //! \brief Override this to scan path and detect new machine data
    virtual int Open(QString path) = 0;

    //! \brief Override to returns the Version number of this MachineLoader
    virtual int Version() = 0;

    static Machine * CreateMachine(MachineInfo info, MachineID id = 0);
    Machine * lookupMachine(QString serial);

    // !\\brief Used internally by loaders, override to return base MachineInfo record
    virtual MachineInfo newInfo() { return MachineInfo(); }

    //! \brief Override to returns the class name of this MachineLoader
    virtual const QString &loaderName() = 0;
    inline MachineType type() { return m_type; }

    void unsupported(Machine * m) {
        Q_ASSERT(m != nullptr);
        m->setUnsupported(true);
        emit machineUnsupported(m);
    }

    void queTask(ImportTask * task);

    void addSession(Session * sess)
    {
        sessionMutex.lock();
        new_sessions[sess->session()] = sess;
        sessionMutex.unlock();
    }

    //! \brief Process Task list using all available threads.
    void runTasks(bool threaded=true);

    int countTasks() { return m_tasklist.size(); }

    inline bool isAborted() { return m_abort; }
    void abort() { m_abort = true; }

    virtual void process() {}

    DeviceStatus status() { return m_status; }
    void setStatus(DeviceStatus status) { m_status = status; }

    QMutex sessionMutex;
    QMutex saveMutex;

    void removeMachine(Machine * m);

    virtual void initChannels() {}
    QPixmap & getPixmap(QString series) {
        QHash<QString, QPixmap>::iterator it = m_pixmaps.find(series);
        if (it != m_pixmaps.end()) {
            return it.value();
        }
        return *genericCPAPPixmap;
    }
    QString getPixmapPath(QString series) {
        QHash<QString, QString>::iterator it = m_pixmap_paths.find(series);
        if (it != m_pixmap_paths.end()) {
            return it.value();
        }
        return genericPixmapPath;
    }

signals:
    void updateProgress(int cnt, int total);
    void machineUnsupported(Machine *);

protected:
    //! \brief Contains a list of Machine records known by this loader
    QList<Machine *> m_machlist;

    static QPixmap * genericCPAPPixmap;

    MachineType m_type;
    QString m_class;
    Profile *m_profile;

    int m_currenttask;
    int m_totaltasks;

    bool m_abort;

    DeviceStatus m_status;

    void finishAddingSessions();
    QMap<SessionID, Session *> new_sessions;

    QHash<QString, QPixmap> m_pixmaps;
    QHash<QString, QString> m_pixmap_paths;

  private:
    QList<ImportTask *> m_tasklist;
};

class CPAPLoader:public MachineLoader
{
    Q_OBJECT
public:
    CPAPLoader() : MachineLoader() {}
    virtual ~CPAPLoader() {}

    virtual QList<ChannelID> eventFlags(Day * day);

    virtual QString PresReliefLabel() { return QString(""); }
    virtual ChannelID PresReliefMode() { return NoChannel; }
    virtual ChannelID PresReliefLevel() { return NoChannel; }
    virtual ChannelID HumidifierConnected() { return NoChannel; }
    virtual ChannelID HumidifierLevel() { return CPAP_HumidSetting; }
    virtual ChannelID CPAPModeChannel() { return CPAP_Mode; }
    virtual void initChannels() {}
};

struct ImportPath
{
    ImportPath() {
        loader = nullptr;
    }
    ImportPath(const ImportPath & copy) {
        loader = copy.loader;
        path = copy.path;
    }
    ImportPath(QString path, MachineLoader * loader) :
        path(path), loader(loader) {}

    QString path;
    MachineLoader * loader;
};


// Put in machine loader class as static??
void RegisterLoader(MachineLoader *loader);
MachineLoader * lookupLoader(Machine * m);
MachineLoader * lookupLoader(QString loaderName);

void DestroyLoaders();

bool compressFile(QString inpath, QString outpath = "");

QList<MachineLoader *> GetLoaders(MachineType mt = MT_UNKNOWN);

#endif //MACHINE_LOADER_H
