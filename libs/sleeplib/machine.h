/*
SleepLib Machine Implementation

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL
*/

#ifndef MACHINE_H
#define MACHINE_H

#include <wx/string.h>
#include <wx/variant.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/ffile.h>
#include <wx/utils.h>
#include <map>
#include <vector>
#include <list>

#include "tinyxml/tinyxml.h"
#include "preferences.h"

using namespace std;

typedef long MachineID;
typedef long SessionID;

class BoundsError {};

enum MachineType: short { MT_UNKNOWN=0,MT_CPAP,MT_OXIMETER,MT_SLEEPSTAGE };
// I wish C++ could extend enums.
// Could be implimented by MachineLoader's register function requesting a block of integer space,
// and a map to name strings.

// Be cautious when extending these.. add to the end of each groups to preserve file formats.
enum MachineCode:
wxInt16 {
    // General Event Codes
    CPAP_Obstructive=0, CPAP_Hypopnea, CPAP_ClearAirway, CPAP_RERA, CPAP_VSnore, CPAP_FlowLimit,
    CPAP_Leak, CPAP_Pressure, CPAP_EAP, CPAP_IAP, CPAP_CSR, CPAP_FlowRate,

    // General CPAP Summary Information
    CPAP_PressureMin=0x80, CPAP_PressureMax, CPAP_RampTime, CPAP_RampStartingPressure, CPAP_Mode, CPAP_PressureReliefType,
    CPAP_PressureReliefSetting, CPAP_HumidifierSetting, CPAP_HumidifierStatus, CPAP_PressureMinAchieved,
    CPAP_PressureMaxAchieved, CPAP_PressurePercentValue, CPAP_PressurePercentName, CPAP_PressureAverage, CPAP_PressureMedian,
    CPAP_LeakMedian,CPAP_LeakMinimum,CPAP_LeakMaximum,CPAP_LeakAverage,CPAP_Duration,

    BIPAP_EAPAverage,BIPAP_IAPAverage,BIPAP_EAPMin,BIPAP_EAPMax,BIPAP_IAPMin,BIPAP_IAPMax,

    // PRS1 Specific Codes
    PRS1_PressurePulse=0x1000, PRS1_VSnore2,
    PRS1_Unknown00, PRS1_Unknown01, PRS1_Unknown08, PRS1_Unknown09, PRS1_Unknown0B,	PRS1_Unknown0E, PRS1_Unknown10, PRS1_Unknown12,
    PRS1_SystemLockStatus, PRS1_SystemResistanceStatus, PRS1_SystemResistanceSetting, PRS1_HoseDiameter, PRS1_AutoOff, PRS1_MaskAlert, PRS1_ShowAHI,

};

enum FlagType:
short { FT_BAR, FT_DOT, FT_SPAN };

enum CPAPMode:
short {
    MODE_CPAP=0,MODE_APAP,MODE_BIPAP,MODE_ASV
};
enum PRTypes:
short {
    PR_NONE=0,PR_CFLEX,PR_CFLEXPLUS,PR_AFLEX,PR_BIFLEX,PR_EPR,PR_SMARTFLEX
};

extern map<MachineCode,wxString> DefaultMCShortNames;
extern map<PRTypes,wxString> PressureReliefNames;
extern map<CPAPMode,wxString> CPAPModeNames;

// These are types supported by wxVariant class. To retain compatability, add to the end of this list only..
enum MCDataType:
wxInt8 { MC_bool=0, MC_long, MC_float, MC_double, MC_string, MC_datetime };

typedef wxInt16 SampleFormat;
typedef float EventDataType;

class Session;
class Profile;

class Day
{
public:
    Day();
    ~Day();
    void AddSession(Session *s);


    EventDataType min(MachineCode code,int field=0);
    EventDataType max(MachineCode code,int field=0);
    EventDataType avg(MachineCode code,int field=0);
    EventDataType sum(MachineCode code,int field=0);
    EventDataType count(MachineCode code);
    EventDataType weighted_avg(MachineCode code,int field=0);

    // Note, the following convert to doubles without considering the consequences fully.
    EventDataType summary_avg(MachineCode code);
    EventDataType summary_min(MachineCode code);
    EventDataType summary_max(MachineCode code);

    const wxDateTime & first(MachineCode code);
    const wxDateTime & last(MachineCode code);
    const wxDateTime & first() { return d_first; };
    const wxDateTime & last() { return d_last; };

    wxTimeSpan total_time();
    float hours() { return total_time().GetSeconds().GetLo()/3600.0; };

    Session *operator [](int i) { return sessions[i]; };

    vector<Session *>::iterator begin() { return sessions.begin(); };
    vector<Session *>::iterator end() { return sessions.end(); };

    size_t size() { return sessions.size(); };

protected:
    vector<Session *> sessions;
    wxDateTime d_first,d_last;
    wxTimeSpan d_totaltime;
private:
    bool d_firstsession;
};

class Machine
{
public:
    Machine(Profile *p,MachineID id=0);
    virtual ~Machine();
//	virtual bool Open(wxString path){};

    bool Load();
    bool Save();

    map<wxDateTime,Day *> day;
    map<SessionID,Session *> sessionlist;
    map<wxString,wxString> properties;

    Session * SessionExists(SessionID session);
    void AddSession(Session *s);

    void SetClass(wxString t) {
        m_class=t;
    };
    void SetType(MachineType t) {
        m_type=t;
    };
    const wxString & GetClass() {
        return m_class;
    };
    const MachineType & GetType() {
        return m_type;
    };
    const wxString hexid() {
        wxString s;
        s.Printf(wxT("%08lx"),m_id);
        return s;
    };
    const MachineID & id() { return m_id; };
    const wxDateTime & FirstDay() { return firstday; };
    const wxDateTime & LastDay() { return lastday; };

//    const wxDateTime & MinTime(const wxDateTime & date);
 //   const wxDateTime & MaxTime(const wxDateTime & date);

protected:
    wxDateTime firstday,lastday;

    MachineID m_id;
    //wxString m_filename;
    wxString m_class;
    MachineType m_type;
    wxString m_path;
    Profile *profile;
    bool changed;
    bool firstsession;
};


class Event
{
    friend class Session;
public:
    Event(wxDateTime time,MachineCode code,list<EventDataType> data);
    ~Event();
    const EventDataType operator[](short i) {
        if (i<e_fields) return e_data[i];
        else return 0;
    };
    const wxDateTime & time() {
        return e_time;
    };
    const MachineCode & code() {
        return e_code;
    };
    const short & fields() {
        return e_fields;
    };
protected:
    wxDateTime e_time;
    MachineCode e_code;
    short e_fields;
    vector<EventDataType> e_data;
};

class Waveform
{
    friend class Session;
public:
    Waveform(wxDateTime time,MachineCode code,SampleFormat * data,int samples,float duration,SampleFormat min, SampleFormat max);
    ~Waveform();
    const SampleFormat operator[](int i) {
        if (i<w_samples) return w_data[i];
        else return 0;
    };
    const wxDateTime & start() {
        return w_time;
    };
    const wxDateTime end() {
        return w_time+w_totalspan;
    };
    const MachineCode & code() {
        return w_code;
    };
    const int & samples() {
        return w_samples;
    };
    const float & duration() {
        return w_duration;
    };
    const SampleFormat & min() {
        return Min;
    };
    const SampleFormat & max() {
        return Max;
    };
    SampleFormat *GetBuffer() { return w_data; };

protected:
    wxDateTime w_time;
    MachineCode w_code;
    SampleFormat * w_data;
    wxInt32 w_samples;
    float w_duration;
    SampleFormat Min,Max;
    wxTimeSpan w_totalspan;
    wxTimeSpan w_samplespan;
};


class Session
{
public:
    Session(Machine *,SessionID);
    virtual ~Session();

    void AddEvent(Event *e);
    void AddWaveform(Waveform *w);

    bool Store(wxString path);
    bool StoreSummary(wxString filename);
    bool StoreEvents(wxString filename);
    bool StoreWaveforms(wxString filename);
    //bool Load(wxString path);
    bool LoadSummary(wxString filename);
    bool LoadEvents(wxString filename);
    bool LoadWaveforms(wxString filename);

    void TrashEvents();
    void TrashWaveforms();

    const SessionID & session() {
        return s_session;
    };
    const wxDateTime & first() {
        return s_first;
    };
    const wxDateTime & last() {
        return s_last;
    };
    void SetSessionID(SessionID s) {
        s_session=s;
    };
    void set_first(wxDateTime d) {
        s_first=d;
    };
    void set_last(wxDateTime d) {
        s_last=d;
    };
    void set_hours(float h) {
        s_hours=h;
    };

    const float & hours() {
        return s_hours;
    };
    int count_events(MachineCode mc) {
        if (events.find(mc)==events.end()) return 0;
        return events[mc].size();
    };
    double min_event_field(MachineCode mc,int field);
    double max_event_field(MachineCode mc,int field);
    double sum_event_field(MachineCode mc,int field);
    double avg_event_field(MachineCode mc,int field);
    double weighted_avg_event_field(MachineCode mc,int field);

    map<MachineCode,wxVariant> summary;
    void SetChanged(bool val) {
        s_changed=val;
    };
    bool IsChanged() {
        return s_changed;
    };
    map<MachineCode,vector<Event *> > events;
    map<MachineCode,vector<Waveform *> > waveforms;

protected:
    SessionID s_session;

    Machine *s_machine;
    wxDateTime s_first;
    wxDateTime s_last;
    float s_hours;
    bool s_changed;
};

class CPAP:public Machine
{
public:
    CPAP(Profile *p,MachineID id=0);
    virtual ~CPAP();
//	virtual bool Open(wxString path);
    map<MachineCode,wxColour> FlagColours;
    map<MachineCode,FlagType> FlagTypes;
    list<MachineCode> SleepFlags;
};

class Oximeter:public Machine
{
public:
    Oximeter(Profile *p,MachineID id=0);
    virtual ~Oximeter();
//	virtual bool Open(wxString path){};
protected:
};

class SleepStage:public Machine
{
public:
    SleepStage(Profile *p,MachineID id=0);
    virtual ~SleepStage();
//	virtual bool Open(wxString path){};
protected:
};


#endif // MACHINE_H

