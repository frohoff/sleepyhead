/********************************************************************
 Custom graph data Headers
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#ifndef GRAPHDATA_CUSTOM_H
#define GRAPHDATA_CUSTOM_H

#include <QDateTime>
#include "SleepLib/profiles.h"
#include "SleepLib/day.h"
#include "SleepLib/machine_common.h"
#include "graphdata.h"

class FlagData:public gPointData
{
public:
    FlagData(MachineCode _code,double _value=0,int _field=-1,int _offset=-1);
    virtual ~FlagData();
    virtual void Reload(Day *day=NULL);
protected:
    MachineCode code;
    double value;
    int field;
    int offset;
};

class TAPData:public gPointData
{
public:
    TAPData(MachineCode _code);
    virtual ~TAPData();
    virtual void Reload(Day *day=NULL);

    static const int max_slots=4096;
    double pTime[max_slots];
    MachineCode code;

};

class WaveData:public gPointData
{
public:
    WaveData(MachineCode _code,int _size=1000000);
    virtual ~WaveData();
    virtual void Reload(Day *day=NULL);
protected:
    MachineCode code;
};

class EventData:public gPointData
{
public:
    EventData(MachineCode _code,int _field=0,int _size=250000,bool _skipzero=false);
    virtual ~EventData();
    virtual void Reload(Day *day=NULL);
protected:
    MachineCode code;
    int field;
    bool skipzero;
};


class AHIData:public gPointData
{
public:
    AHIData();
    virtual ~AHIData();
    virtual void Reload(Day *day=NULL);
};

class HistoryData:public gPointData
{
public:
    HistoryData(Profile * _profile);
    virtual ~HistoryData();

    void SetProfile(Profile *_profile) { profile=_profile; Reload(); };
    Profile * GetProfile() { return profile; };
    double GetAverage();

    virtual double Calc(Day *day);
    virtual void Reload(Day *day=NULL);
    virtual void ResetDateRange();
    virtual void SetDateRange(QDate start,QDate end);
  //  virtual void Reload(Machine *machine=NULL);
protected:
    Profile * profile;
};

class HistoryCodeData:public HistoryData
{
public:
    HistoryCodeData(Profile *_profile,MachineCode _code);
    virtual ~HistoryCodeData();
    virtual double Calc(Day *day);
protected:
    MachineCode code;
};


enum T_UHD { UHD_Bedtime, UHD_Waketime, UHD_Hours };
class UsageHistoryData:public HistoryData
{
public:
    UsageHistoryData(Profile *_profile,T_UHD _uhd);
    virtual ~UsageHistoryData();
    virtual double Calc(Day *day);
protected:
    T_UHD uhd;
};


#endif // GRAPHDATA_CUSTOM_H
