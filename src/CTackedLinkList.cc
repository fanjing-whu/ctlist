/*
 * CTackedLinkList.cpp
 *
 *  Created on: 2013-9-29
 *      Author: Fanjing-LAB
 */
#include "common.h"
#include "helper.h"
//#include "bplus.h"
#include "ctlink.h"
#include "CILink.h"
#include "CArrayList.h"
#include "Generator.h"
#include "PreciseTimer.h"
#include "ASMTimer.h"
#include "RecordTools.h"
#include "StringHelper.h"
//#include "stdlib.h"
using namespace Fanjing;
/*
 *
 unsigned int g_BW_Down = 1;
 unsigned int g_BW_Up = 20;
 unsigned int g_TS_Down = 0;
 unsigned int g_TS_Up = 604800; 86400;
 unsigned int g_TD_Down = 1;
 unsigned int g_TD_Up = 32768;
 unsigned int g_Interval_Avg = 50;
 unsigned int g_Index_Interval = g_Interval_Avg * 5;
 */
static unsigned int DEFALUT_SLOT_SIZE0 = 1;
static unsigned int DEFALUT_SLOT_SIZE1 = 8;
static unsigned int DEFALUT_BW_DOWN = 1;
static unsigned int DEFALUT_BW_UP = 200;
static unsigned int DEFALUT_TS_DOWN = 1;
static unsigned int DEFALUT_TS_UP = 86400;
static unsigned int DEFALUT_TD_DOWN = 1;
static unsigned int DEFALUT_TD_UP = 512;
static unsigned int DEFALUT_INTERVAL_AVG = 75;
static double DEFALUT_INDEX_MULTIPLE = 2;
static double globalRSfixedARRAY[12] = {
        2.25,
        8.00,
        16.0,
        1.00,
        1,
        1,
        2.75,
        10,
        19.8,
        0.43,
        3.4,
        19.8 };
static int DEFAULT_MAX_NROUND = 3;
template<class T> string m_toStr(T tmp) {
    stringstream ss;
    ss << tmp;
    return ss.str();
}

struct ControlStack {
    // synchronizer between main thread and record thread.
    unsigned int multiple;
    unsigned int n; // the num of requests processed
    unsigned int t; // the seconds passed
    string logName; // the name of the log file
    bool stopFlag;
};

DWORD WINAPI RecordFor5(LPVOID tmp) {
    ControlStack* p = (ControlStack*) tmp;
    unsigned int pre_num = 0;
    unsigned int interval = 1000 / p->multiple;
    while(1){
        if(p->stopFlag){
            break;
        }
        ofstream file(p->logName.c_str(), ios::app);
        p->t++;
        file << p->t << "\t" << p->n << "\t" << p->n - pre_num << endl;
        pre_num = p->n;
        file.close();
        Sleep(interval); //ms
    }
    return 0;
}
string dbl2Str(double dou, int precision = -1, bool useSignOfPercent = false) {
    stringstream ss;
    ss.setf(std::ios::fixed);
    if(precision >= 0){
        ss.precision(precision);
    }else{
        ss.precision(0);
    }
    if(useSignOfPercent){
        ss << dou * 100;
        return ss.str() + "%";
    }else{
        ss << dou;
        return ss.str();
    }
}

string dbl2Pcent(double dou, int precision = 2) {
    return dbl2Str(dou, precision, true);
}

string statistics2Str(double dou0, double dou1, int precision = 2) {
    return dbl2Str(dou0) + "(" + dbl2Pcent(dou0 / dou1) + ")";
    //return dbl2Str(dou0);
}

void exASMTest() {
    ASMTimer* at = ASMTimer::request();
    at->start();
    at->end();
    cout << at->getCounts() << ":" << endl;
    int a = 200;
    at->start();
    Sleep(a);
    at->end();
    cout << at->getMilliseconds() << "ms@" << at->getFrequency() / 1000000000.0
            << "GHz" << endl;
    at->release();
}
void exHelperTest() {
    Helper h;
    int n = 20;
    cout << h.F_Rand(2, pow(2, n)) << endl;
    for(int i = 0; i < n; ++i){
        cout << h.F_Rand(2, pow(2, 10)) << endl;
    }
}
void exDevelopTest() {
    Generator* gn = new Generator();
    gn->setGenerator(1, 20, 0, 36000, 800, 36000, 10);
    BaseAdmissionController* ct;
    ASMTimer* timer = ASMTimer::request();
    unsigned int t_total0 = 0;
    unsigned int t_total1 = 0;
    unsigned int t_setTime = 0;
    unsigned int t_accept = 0;
    unsigned int t_storage = 0;
    unsigned int n_accetp0 = 0;
    unsigned int n_accetp1 = 0;
    unsigned int curTime = 0;
    Request* r = new Request[100000];
    unsigned int* interval = new unsigned int[100000];

    for(int i = 0; i < 100000; i++){
        interval[i] = gn->getNext(&r[i]);
    }
    cout << r[1000].value << endl;
    for(int n = 0; n < 2; ++n){
        Sleep(50);
        //t_total = 0;
        t_setTime = 0;
        t_accept = 0;
        t_storage = 0;
        curTime = 0;
        CTLink* ct0 = new CTLink(720, 72000);
        ct0->iMaxResource = 200;
        CILink* ci0 = new CILink(720, 72000);
        ci0->iMaxResource = 200;
        if(n % 2 == 0){
            ct = dynamic_cast<BaseAdmissionController*>(ct0);
        }else{
            ct = dynamic_cast<BaseAdmissionController*>(ci0);
        }
        //Request temp;
        bool flag = true;
        for(int i = 0; i < 1000; i++){
            curTime += interval[i];
            timer->start();
            ct->setTime(curTime);
            timer->end();
            t_setTime += timer->getCounts();
            timer->start();
            flag = ct->accept(r[i]);
            timer->end();
            t_accept += timer->getCounts();
            if(flag){
                timer->start();
                ct->forceInsert(r[i]);
                timer->end();
                t_storage += timer->getCounts();
                if(n % 2 == 0){
                    n_accetp0++;
                }else{
                    n_accetp1++;
                }
            }
        }
        if(n % 2 == 0){
            t_total0 += (t_setTime + t_storage + t_accept) / 10;
        }else{
            t_total1 += (t_setTime + t_storage + t_accept) / 10;
        }
        cout << (n % 2 == 0 ? "ct:" : "ci:") << endl;
        cout << "settime: " << t_setTime << endl;
        cout << "accept: " << t_accept << endl;
        cout << "storage: " << t_storage << endl;
        delete ct0;
        delete ci0;
    }
    cout << "ct: " << t_total0 << "\t" << n_accetp0 << endl;
    cout << "ci: " << t_total1 << "\t" << n_accetp1 << endl;
    delete[] interval;
    delete[] r;
    delete gn;
}
void exCommonTest(string filename) {
    // statistics parameters
    unsigned int s_Interval = 100;
    unsigned int s_Request_Num = 10000;
    // try to minimum sample error which is cause by the time slot of the process
    unsigned int n_repeatTimes = 5; // repeat times of each statistics;
    unsigned int n_sample = 4;  // the number of samples
    // requests parameters
    unsigned int g_BW_Down = DEFALUT_BW_DOWN;
    unsigned int g_BW_Up = DEFALUT_BW_UP;
    unsigned int g_TS_Down = DEFALUT_TS_DOWN;
    unsigned int g_TS_Up = DEFALUT_TS_UP;
    unsigned int g_TD_Down = DEFALUT_TD_DOWN;
    unsigned int g_TD_Up = DEFALUT_TD_UP;
    unsigned int g_Interval_Avg = DEFALUT_INTERVAL_AVG;
    unsigned int g_Index_Interval = g_Interval_Avg * DEFALUT_INDEX_MULTIPLE;
    cout << "g_Index_Interval;" << g_Index_Interval << endl;
    cout << "g_Interval_Avg;" << g_Interval_Avg << endl;
    cout << "nodesPerIndex;" << g_Index_Interval * 1.0 / g_Interval_Avg << endl;
    // initialize the test units
    BaseAdmissionController** ct = new BaseAdmissionController*[n_repeatTimes
            * n_sample];
    ASMTimer* timer = ASMTimer::request();
    RecordTools* stool = RecordTools::request();
    stool->setDefaultDir("results");
    Generator* gn = new Generator();
    Request* r = new Request[s_Request_Num];
    unsigned int* interval = new unsigned int[s_Request_Num];
    double r_radio = 0.8;
    g_TD_Up = 512;
    // run the experiment under n_round different settings : TD max~ 512,4096,32768
    for(int n_round = 0; n_round < DEFAULT_MAX_NROUND; ++n_round, g_TD_Up *= 8){
        gn->setGenerator(g_BW_Down, g_BW_Up, g_TS_Down, g_TS_Up, g_TD_Down,
                g_TD_Up, g_Interval_Avg);
        unsigned int max_resource = (g_BW_Down + g_BW_Up) / 2 * g_TD_Up
                / globalRSfixedARRAY[0 + n_round] / g_Interval_Avg * r_radio;
        // set requests and the intervals
        for(unsigned int i = 0; i < s_Request_Num; i++){
            interval[i] = gn->getNext(&r[i]);
        }
        // initialize the storages: 0~CArray,1~CArray8,2~CIlink,3~CTlink
        unsigned int max_range = g_TS_Up + g_TD_Up;
        unsigned int index_num = max_range / g_Index_Interval;
        for(unsigned int n = 0; n < n_repeatTimes; ++n){
            unsigned int n_no = 0;
            ct[n * n_sample + n_no] = new CArrayList(max_range,
                    DEFALUT_SLOT_SIZE0);
            ct[n * n_sample + n_no]->setResourceCap(max_resource);
            n_no++;
            ct[n * n_sample + n_no] = new CArrayList(max_range,
                    DEFALUT_SLOT_SIZE1);
            ct[n * n_sample + n_no]->setResourceCap(max_resource);
            n_no++;
            ct[n * n_sample + n_no] = new CILink(index_num, max_range);
            ct[n * n_sample + n_no]->setResourceCap(max_resource);
            n_no++;
            ct[n * n_sample + n_no] = new CTLink(index_num, max_range);
            ct[n * n_sample + n_no]->setResourceCap(max_resource);
        }

        // set temporary statistics variables
        unsigned int *t_SetTime = new unsigned int[n_sample];
        memset(t_SetTime, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_Accept = new unsigned int[n_sample];
        memset(t_Accept, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_Storage = new unsigned int[n_sample];
        memset(t_Storage, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_MinSetTime = new unsigned int[n_sample];
        memset(t_MinSetTime, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_MinAccept = new unsigned int[n_sample];
        memset(t_MinAccept, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_MinStorage = new unsigned int[n_sample];
        memset(t_MinStorage, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_TSetTime = new unsigned int[n_sample];
        memset(t_TSetTime, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_TAccept = new unsigned int[n_sample];
        memset(t_TAccept, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_TStorage = new unsigned int[n_sample];
        memset(t_TStorage, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_Total = new unsigned int[n_sample];
        memset(t_Total, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_nAccept = new unsigned int[n_sample];
        memset(t_nAccept, 0, n_sample * sizeof(unsigned int));

        // (s_Request_Num+s_Interval-1)/s_Interval rounds out circle
        unsigned int outCircleNum = (s_Request_Num + s_Interval - 1)
                / s_Interval;
        unsigned int endInnerCircle = s_Interval;
        unsigned int startInnerCircle = 0;
        unsigned int curCircleNum = 0;
        unsigned int oldTime = 0;
        unsigned int curTime = 0;
        bool flag = false;
        // initial fill up phase
        if(true){
            // pre-filled up if necessary
            unsigned int n_num = n_repeatTimes * n_sample;
            unsigned int n_prefillup_num = 10000;
            if(n_prefillup_num > s_Request_Num){
                n_prefillup_num = s_Request_Num;
            }
            for(unsigned int n = 0; n < n_num; ++n){
                curTime = oldTime;
                for(unsigned int n_pfup = 0; n_pfup < n_prefillup_num;
                        ++n_pfup){
                    curTime += interval[n_pfup];
                    ct[n]->setTime(curTime);
                    flag = ct[n]->accept(r[n_pfup]);
                    if(flag){
                        ct[n]->forceInsert(r[n_pfup]);
                    }
                }
            }
            oldTime = curTime;
        }
        Sleep(5);
        for(unsigned int ocn = 0; ocn < outCircleNum; ocn++){
            // inner circle for different storage types;
            for(unsigned int n_repeat = 0; n_repeat < n_repeatTimes;
                    ++n_repeat){
                for(unsigned int n_type = 0; n_type < n_sample; ++n_type){
                    // release cpu before each loop
                    // Sleep(5);
                    curTime = oldTime;
                    curCircleNum = startInnerCircle;
                    for(; curCircleNum < endInnerCircle; curCircleNum++){
                        if(curCircleNum >= s_Request_Num){
                            cout << "ERROR!!" << endl;
                            return;
                        }
                        // using temporary variables to avoid index conculation
                        BaseAdmissionController* testSuit = ct[n_repeat
                                * n_sample + n_type];
                        Request testRequest = r[curCircleNum];
                        // run and statistics
                        curTime += interval[curCircleNum];
                        timer->start();
                        testSuit->setTime(curTime);
                        timer->end();
                        t_SetTime[n_type] += timer->getCounts();
                        timer->start();
                        flag = testSuit->accept(testRequest);
                        timer->end();
                        t_Accept[n_type] += timer->getCounts();
                        if(flag){
                            timer->start();
                            testSuit->forceInsert(testRequest);
                            timer->end();
                            t_Storage[n_type] += timer->getCounts();
                            // the accept time just add once
                            if(n_repeat == 0){
                                t_nAccept[n_type]++;
                            }
                        }
                    }
                    curCircleNum = startInnerCircle;
                    if(n_repeat != 0){
                        t_MinSetTime[n_type] =
                                t_MinSetTime[n_type] < t_SetTime[n_type] ? t_MinSetTime[n_type] :
                                        t_SetTime[n_type];
                        t_MinAccept[n_type] =
                                t_MinAccept[n_type] < t_Accept[n_type] ? t_MinAccept[n_type] :
                                        t_Accept[n_type];
                        t_MinStorage[n_type] =
                                t_MinStorage[n_type] < t_Storage[n_type] ? t_MinStorage[n_type] :
                                        t_Storage[n_type];
                    }else{
                        t_MinSetTime[n_type] = t_SetTime[n_type];
                        t_MinAccept[n_type] = t_Accept[n_type];
                        t_MinStorage[n_type] = t_Storage[n_type];
                    }
                }
                for(unsigned int n_type = 0; n_type < n_sample; ++n_type){
                    t_SetTime[n_type] = 0;
                    t_Accept[n_type] = 0;
                    t_Storage[n_type] = 0;
                }
            }
            // statistics process
            stringstream ss;
            string name;
            ss << "n_round,limit,outCircleNum(ocn),round(endInnerCircle)";
            for(unsigned int n_type = 0; n_type < n_sample; ++n_type){
                ss << ",settime-" << n_type << ",accept-" << n_type
                        << ",storage-" << n_type << ",naccept-" << n_type
                        << ",total-" << n_type << "";
            }
            name = ss.str();
            ss.str("");
            string fileSuffix;
            fileSuffix = fileSuffix + "cost of ocn rounds"
                    + Fanjing::StringHelper::int2str(g_TD_Up);
            stool->changeName(fileSuffix, name) << n_round << g_TD_Up << ocn
                    << endInnerCircle;
            for(unsigned int n_type = 0; n_type < n_sample; ++n_type){
                stool->get() << t_MinSetTime[n_type] << t_MinAccept[n_type]
                        << t_MinStorage[n_type] << t_nAccept[n_type]
                        << t_MinSetTime[n_type] + t_MinAccept[n_type]
                                + t_MinStorage[n_type];
            }
            stool->get() << stool->endl;
            // set statistics variables
            for(unsigned int n_type = 0; n_type < n_sample; ++n_type){
                t_TSetTime[n_type] += t_MinSetTime[n_type];
                t_TAccept[n_type] += t_MinAccept[n_type];
                t_TStorage[n_type] += t_MinStorage[n_type];
                t_Total[n_type] = t_TSetTime[n_type] + t_TAccept[n_type]
                        + t_TStorage[n_type];
            }
            // end the loop
            oldTime = curTime;
            startInnerCircle = endInnerCircle;
            endInnerCircle =
                    endInnerCircle + s_Interval > s_Request_Num ? s_Request_Num :
                            endInnerCircle + s_Interval;
        }
        stringstream ss;
        string name;
        ss << "n_round,g_TD_Limit";
        for(unsigned int n_type = 0; n_type < n_sample; ++n_type){
            ss << ",settime-" << n_type << ",accept-" << n_type << ",storage-"
                    << n_type << ",naccept-" << n_type << ",total-" << n_type
                    << "";
        }
        name = ss.str();
        ss.str("");
        stool->changeName("finally cost until n_round", name) << n_round
                << g_TD_Up;
        for(unsigned int n_type = 0; n_type < n_sample; ++n_type){
            stool->get()
                    << statistics2Str(t_TSetTime[n_type],
                            (1.0 * t_Total[n_type]))
                    << statistics2Str(t_TAccept[n_type],
                            (1.0 * t_Total[n_type]))
                    << statistics2Str(t_TStorage[n_type],
                            (1.0 * t_Total[n_type]))
                    << statistics2Str(t_nAccept[n_type], (1.0 * s_Request_Num))
                    << t_Total[n_type];
        }
        cout << "AcceptRatio:" << t_nAccept[3] * 1.0 / s_Request_Num << endl;
        stool->get() << stool->endl;
        string filePrefix = "totalCost_";
        for(unsigned int n_type = 0; n_type < n_sample; ++n_type){
            stool->changeName(
                    filePrefix + Fanjing::StringHelper::dbl2str(g_TD_Up, 2)
                            +"_"+ Fanjing::StringHelper::int2str(n_type),
                    "type\tcost") << n_type << t_Total[n_type] << stool->endl;
        }

        // destruction
        delete[] t_SetTime;
        delete[] t_Accept;
        delete[] t_Storage;
        delete[] t_MinSetTime;
        delete[] t_MinAccept;
        delete[] t_MinStorage;
        delete[] t_TSetTime;
        delete[] t_TAccept;
        delete[] t_TStorage;
        delete[] t_Total;
        delete[] t_nAccept;
        for(unsigned int i = 0; i < n_repeatTimes * n_sample; ++i){
            if(i == 2 || i == 3){
                cout << ct[i]->Output() << endl;
            }
            delete ct[i];
        }
    }
    stool->outputSeparate(filename + ".txt");
    stool->clean();
    // destruction
    delete gn;
    delete[] r;
    delete[] interval;
    delete[] ct;
}
void exStartPhaseTest(string filename) {
    // FIXMEED chenge the code
    // statistics parameters
    unsigned int s_Interval = 1;
    unsigned int s_Request_Num = 100;
    // try to minimum sample error which is cause by the time slot of the process
    unsigned int n_repeatTimes = 10; // repeat times of each statistics;
    unsigned int n_sample = 4;  // the number of samples
    unsigned int n_multiple = 50;
    unsigned int n_prefillup_num = 10000;
    unsigned int n_TotalRequest_Num = s_Request_Num * n_multiple;
    // requests parameters
    unsigned int g_BW_Down = DEFALUT_BW_DOWN;
    unsigned int g_BW_Up = DEFALUT_BW_UP;
    unsigned int g_TS_Down = DEFALUT_TS_DOWN;
    unsigned int g_TS_Up = DEFALUT_TS_UP;
    unsigned int g_TD_Down = DEFALUT_TD_DOWN;
    unsigned int g_TD_Up = DEFALUT_TD_UP;
    unsigned int g_Interval_Avg = DEFALUT_INTERVAL_AVG;
    unsigned int g_Index_Interval = g_Interval_Avg * DEFALUT_INDEX_MULTIPLE;
    cout << "g_Index_Interval;" << g_Index_Interval << endl;
    cout << "g_Interval_Avg;" << g_Interval_Avg << endl;
    cout << "nodesPerIndex;" << g_Index_Interval * 1.0 / g_Interval_Avg << endl;
    // initialize the test units
    BaseAdmissionController** ct = new BaseAdmissionController*[n_repeatTimes
            * n_sample * n_multiple];
    ASMTimer* timer = ASMTimer::request();
    RecordTools* stool = RecordTools::request();
    stool->setDefaultDir("results");
    Generator* gn = new Generator();
    Request* r = new Request[n_TotalRequest_Num];
    unsigned int* interval = new unsigned int[n_TotalRequest_Num];
    double r_radio = 1.0;
    // run the experiment under n_round different settings : TD max~ 512,4096,32768
    for(int n_round = 0; n_round < DEFAULT_MAX_NROUND; ++n_round, g_TD_Up *= 8){
        gn->setGenerator(g_BW_Down, g_BW_Up, g_TS_Down, g_TS_Up, g_TD_Down,
                g_TD_Up, g_Interval_Avg);
        unsigned int max_resource = (g_BW_Down + g_BW_Up) / 2 * g_TD_Up
                / globalRSfixedARRAY[3 + n_round] / g_Interval_Avg * r_radio;
        // set requests and the intervals
        for(unsigned int i = 0; i < n_TotalRequest_Num; i++){
            interval[i] = gn->getNext(&r[i]);
        }
        // initialize the storages: 0~CArray,1~CArray8,2~CIlink,3~CTlink
        unsigned int max_range = g_TS_Up + g_TD_Up;
        unsigned int index_num = max_range / g_Index_Interval;
        for(unsigned int i_repeatTimes = 0; i_repeatTimes < n_repeatTimes;
                ++i_repeatTimes){
            for(unsigned int i_multiple = 0; i_multiple < n_multiple;
                    ++i_multiple){
                unsigned int i_sample = 0;
                // {repeatTime:0{sample:0{multiple:0~n}~n}~n}
                ct[i_repeatTimes * n_sample * n_multiple + i_sample * n_multiple
                        + i_multiple] = new CArrayList(max_range,
                        DEFALUT_SLOT_SIZE0);
                ct[i_repeatTimes * n_sample * n_multiple + i_sample * n_multiple
                        + i_multiple]->setResourceCap(max_resource);
                i_sample++;
                ct[i_repeatTimes * n_sample * n_multiple + i_sample * n_multiple
                        + i_multiple] = new CArrayList(max_range,
                        DEFALUT_SLOT_SIZE1);
                ct[i_repeatTimes * n_sample * n_multiple + i_sample * n_multiple
                        + i_multiple]->setResourceCap(max_resource);
                i_sample++;
                ct[i_repeatTimes * n_sample * n_multiple + i_sample * n_multiple
                        + i_multiple] = new CILink(index_num, max_range);
                ct[i_repeatTimes * n_sample * n_multiple + i_sample * n_multiple
                        + i_multiple]->setResourceCap(max_resource);
                i_sample++;
                ct[i_repeatTimes * n_sample * n_multiple + i_sample * n_multiple
                        + i_multiple] = new CTLink(index_num, max_range);
                ct[i_repeatTimes * n_sample * n_multiple + i_sample * n_multiple
                        + i_multiple]->setResourceCap(max_resource);
                i_sample++;
            }
        }

        // set temporary step-by-step statistics variables
        unsigned int *t_SetTime = new unsigned int[n_sample];
        memset(t_SetTime, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_Accept = new unsigned int[n_sample];
        memset(t_Accept, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_Storage = new unsigned int[n_sample];
        memset(t_Storage, 0, n_sample * sizeof(unsigned int));
        // min step-by-step statistics variables
        unsigned int *t_MinSetTime = new unsigned int[n_sample];
        memset(t_MinSetTime, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_MinAccept = new unsigned int[n_sample];
        memset(t_MinAccept, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_MinStorage = new unsigned int[n_sample];
        memset(t_MinStorage, 0, n_sample * sizeof(unsigned int));
        // multiple step-by-step statistics variables
        unsigned int *t_MultiMinSetTime = new unsigned int[n_sample];
        memset(t_MultiMinSetTime, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_MultiMinAccept = new unsigned int[n_sample];
        memset(t_MultiMinAccept, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_MultiMinStorage = new unsigned int[n_sample];
        memset(t_MultiMinStorage, 0, n_sample * sizeof(unsigned int));
        // out-circle multiple statistics variables
        unsigned int *t_OutMultiMinSetTime = new unsigned int[n_sample];
        memset(t_OutMultiMinSetTime, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_OutMultiMinAccept = new unsigned int[n_sample];
        memset(t_OutMultiMinAccept, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_OutMultiMinStorage = new unsigned int[n_sample];
        memset(t_OutMultiMinStorage, 0, n_sample * sizeof(unsigned int));
        // total statistics variables
        unsigned int *t_TSetTime = new unsigned int[n_sample];
        memset(t_TSetTime, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_TAccept = new unsigned int[n_sample];
        memset(t_TAccept, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_TStorage = new unsigned int[n_sample];
        memset(t_TStorage, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_Total = new unsigned int[n_sample];
        // accumulative statistics variables
        memset(t_Total, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_nAccept = new unsigned int[n_sample];
        memset(t_nAccept, 0, n_sample * sizeof(unsigned int));

        // (s_Request_Num+s_Interval-1)/s_Interval rounds out circle
        unsigned int outCircleNum = (s_Request_Num + s_Interval - 1)
                / s_Interval;
        unsigned int endInnerCircle = s_Interval;
        unsigned int startInnerCircle = 0;
        unsigned int curCircleNum = 0;
        unsigned int oldTime = 0;
        unsigned int curTime = 0;
        bool flag = false;
        // initial fill up phase
        if(false){
            // pre-filled up if necessary
            unsigned int n_num = n_repeatTimes * n_sample;
            if(n_prefillup_num > s_Request_Num){
                n_prefillup_num = s_Request_Num;
            }
            for(unsigned int n = 0; n < n_num; ++n){
                curTime = oldTime;
                for(unsigned int n_pfup = 0; n_pfup < n_prefillup_num;
                        ++n_pfup){
                    curTime += interval[n_pfup];
                    ct[n]->setTime(curTime);
                    flag = ct[n]->accept(r[n_pfup]);
                    if(flag){
                        ct[n]->forceInsert(r[n_pfup]);
                    }
                }
            }
            oldTime = curTime;
        }
        for(unsigned int ocn = 0; ocn < outCircleNum; ocn++){
            // inner circle for different storage types;
            // for each circle, run multiple times with different
            // requests: request[i_Request_Num*s_Request_Num+i_multiple]
            // to get the average cost in average situation.
            // release cpu before each loop
            // Sleep(5);
            curCircleNum = startInnerCircle;
            for(; curCircleNum < endInnerCircle; curCircleNum++){
                if(curCircleNum >= s_Request_Num){
                    cout << "ERROR!!" << endl;
                    return;
                }
                for(unsigned int i_multiple = 0; i_multiple < n_multiple;
                        ++i_multiple){
                    // run and statistics
                    for(unsigned int i_sample = 0; i_sample < n_sample;
                            ++i_sample){
                        for(unsigned int i_repeatTimes = 0;
                                i_repeatTimes < n_repeatTimes; ++i_repeatTimes){
                            // using temporary variables to avoid index conculation
                            BaseAdmissionController* testSuit = ct[i_repeatTimes
                                    * n_sample * n_multiple
                                    + i_sample * n_multiple + i_multiple];
                            Request testRequest = r[curCircleNum * n_multiple
                                    + i_multiple];
                            curTime = oldTime;
                            // using same interval in each one of multiple
                            curTime += interval[curCircleNum];
                            timer->start();
                            testSuit->setTime(curTime);
                            timer->end();
                            t_SetTime[i_sample] = timer->getCounts();
                            timer->start();
                            flag = testSuit->accept(testRequest);
                            timer->end();
                            t_Accept[i_sample] = timer->getCounts();
                            if(flag){
                                timer->start();
                                testSuit->forceInsert(testRequest);
                                timer->end();
                                t_Storage[i_sample] = timer->getCounts();
                                // the accept time just add once
                                if(i_repeatTimes == 0){
                                    t_nAccept[i_sample]++;
                                }
                            }
                            if(i_repeatTimes == 0){
                                t_MinSetTime[i_sample] = t_SetTime[i_sample];
                                t_MinAccept[i_sample] = t_Accept[i_sample];
                                t_MinStorage[i_sample] = t_Storage[i_sample];
                            }else{
                                t_MinSetTime[i_sample] =
                                        t_MinSetTime[i_sample]
                                                < t_SetTime[i_sample] ? t_MinSetTime[i_sample] :
                                                t_SetTime[i_sample];
                                t_MinAccept[i_sample] =
                                        t_MinAccept[i_sample]
                                                < t_Accept[i_sample] ? t_MinAccept[i_sample] :
                                                t_Accept[i_sample];
                                t_MinStorage[i_sample] =
                                        t_MinStorage[i_sample]
                                                < t_Storage[i_sample] ? t_MinStorage[i_sample] :
                                                t_Storage[i_sample];
                            }
                        }
                        if(i_multiple == 0){
                            t_MultiMinSetTime[i_sample] =
                                    t_MinSetTime[i_sample];
                            t_MultiMinAccept[i_sample] = t_MinAccept[i_sample];
                            t_MultiMinStorage[i_sample] =
                                    t_MinStorage[i_sample];
                        }else{
                            t_MultiMinSetTime[i_sample] +=
                                    t_MinSetTime[i_sample];
                            t_MultiMinAccept[i_sample] += t_MinAccept[i_sample];
                            t_MultiMinStorage[i_sample] +=
                                    t_MinStorage[i_sample];
                        }
                    }
                }
                oldTime = curTime;
                for(unsigned int i_sample = 0; i_sample < n_sample; ++i_sample){
                    t_MultiMinSetTime[i_sample] = t_MultiMinSetTime[i_sample]
                            / n_multiple;
                    t_MultiMinAccept[i_sample] = t_MultiMinAccept[i_sample]
                            / n_multiple;
                    t_MultiMinStorage[i_sample] = t_MultiMinStorage[i_sample]
                            / n_multiple;
                    t_OutMultiMinSetTime[i_sample] +=
                            t_MultiMinSetTime[i_sample];
                    t_OutMultiMinAccept[i_sample] += t_MultiMinAccept[i_sample];
                    t_OutMultiMinStorage[i_sample] +=
                            t_MultiMinStorage[i_sample];
                }
            }
            // statistics process
            stringstream ss;
            string name;
            ss << "cost of ocn rounds-" << g_TD_Up
                    << ":n_round,limit,outCircleNum(ocn),round(endInnerCircle)";
            for(unsigned int n_type = 0; n_type < n_sample; ++n_type){
                ss << ",settime-" << n_type << ",accept-" << n_type
                        << ",storage-" << n_type << ",naccept-" << n_type
                        << ",total-" << n_type << "";
            }
            name = ss.str();
            ss.str("");
            stool->changeName(name) << n_round << g_TD_Up << ocn
                    << endInnerCircle;
            for(unsigned int i_sample = 0; i_sample < n_sample; ++i_sample){
                // outMultiMinXXX: the total average min??? time of one inner circle.
                // nAccept: the total accepted requests; nAccept/multiple: the average accepted requests for one multiple.
                // at last, reset outMultiMinXXX.
                stool->get() << t_OutMultiMinSetTime[i_sample]
                        << t_OutMultiMinAccept[i_sample]
                        << t_OutMultiMinStorage[i_sample]
                        << t_nAccept[i_sample] / n_multiple
                        << t_OutMultiMinSetTime[i_sample]
                                + t_OutMultiMinAccept[i_sample]
                                + t_OutMultiMinStorage[i_sample];
                t_OutMultiMinSetTime[i_sample] = 0;
                t_OutMultiMinAccept[i_sample] = 0;
                t_OutMultiMinStorage[i_sample] = 0;
            }
            stool->get() << stool->endl;
            // set statistics variables
            for(unsigned int i_sample = 0; i_sample < n_sample; ++i_sample){
                t_TSetTime[i_sample] += t_MinSetTime[i_sample];
                t_TAccept[i_sample] += t_MinAccept[i_sample];
                t_TStorage[i_sample] += t_MinStorage[i_sample];
                t_Total[i_sample] = t_TSetTime[i_sample] + t_TAccept[i_sample]
                        + t_TStorage[i_sample];
            }
            // end the loop
            startInnerCircle = endInnerCircle;
            endInnerCircle =
                    endInnerCircle + s_Interval > s_Request_Num ? s_Request_Num :
                            endInnerCircle + s_Interval;
        }
        stringstream ss;
        string name;
        ss << "finally cost until n_round:n_round,g_TD_Limit";
        for(unsigned int n_type = 0; n_type < n_sample; ++n_type){
            ss << ",settime-" << n_type << ",accept-" << n_type << ",storage-"
                    << n_type << ",naccept-" << n_type << ",total-" << n_type
                    << "";
        }
        name = ss.str();
        ss.str("");
        stool->changeName(name) << n_round << g_TD_Up;
        for(unsigned int n_type = 0; n_type < n_sample; ++n_type){
            stool->get()
                    << statistics2Str(t_TSetTime[n_type],
                            (1.0 * t_Total[n_type]))
                    << statistics2Str(t_TAccept[n_type],
                            (1.0 * t_Total[n_type]))
                    << statistics2Str(t_TStorage[n_type],
                            (1.0 * t_Total[n_type]))
                    << statistics2Str(t_nAccept[n_type],
                            (n_multiple * 1.0 * s_Request_Num))
                    << t_Total[n_type];
        }
        cout << "AcceptRatio:" << t_nAccept[3] * 1.0 / s_Request_Num << endl;
        stool->get() << stool->endl;
        // destruction
        // set temporary step-by-step statistics variables
        delete[] t_SetTime;
        delete[] t_Accept;
        delete[] t_Storage;
        // min step-by-step statistics variables
        delete[] t_MinSetTime;
        delete[] t_MinAccept;
        delete[] t_MinStorage;
        // multiple step-by-step statistics variables
        delete[] t_MultiMinSetTime;
        delete[] t_MultiMinAccept;
        delete[] t_MultiMinStorage;
        // out-circle multiple statistics variables
        delete[] t_OutMultiMinSetTime;
        delete[] t_OutMultiMinAccept;
        delete[] t_OutMultiMinStorage;
        // total statistics variables
        delete[] t_TSetTime;
        delete[] t_TAccept;
        delete[] t_TStorage;
        delete[] t_Total;
        // accumulative statistics variables
        delete[] t_nAccept;
        for(unsigned int i = 0; i < n_repeatTimes * n_sample * n_multiple; ++i){
            delete ct[i];
        }
    }
    stool->outputSeparate(filename + ".txt");
    stool->clean();
    // destruction
    delete gn;
    delete[] r;
    delete[] interval;
    delete[] ct;
}
void exUnlanceTest(string filename) {
    // statistics parameters
    unsigned int s_Interval = 100;
    unsigned int s_Request_Num = 10000;
    // try to minimum sample error which is cause by the time slot of the process
    unsigned int n_repeatTimes = 10; // repeat times of each statistics;
    unsigned int n_sample = 4;  // the number of samples
    // requests parameters
    unsigned int g_BW_Down = DEFALUT_BW_DOWN;
    unsigned int g_BW_Up = DEFALUT_BW_UP;
    unsigned int g_TS_Down = DEFALUT_TS_DOWN;
    unsigned int g_TS_Up = DEFALUT_TS_UP;
    unsigned int g_TD_Down = DEFALUT_TD_DOWN;
    unsigned int g_TD_Up = DEFALUT_TD_UP;
    unsigned int g_Interval_Avg = DEFALUT_INTERVAL_AVG;
    unsigned int g_Index_Interval = g_Interval_Avg * DEFALUT_INDEX_MULTIPLE;
    cout << "g_Index_Interval;" << g_Index_Interval << endl;
    cout << "g_Interval_Avg;" << g_Interval_Avg << endl;
    cout << "nodesPerIndex;" << g_Index_Interval * 1.0 / g_Interval_Avg << endl;
    // initialize the test units
    BaseAdmissionController** ct = new BaseAdmissionController*[n_repeatTimes
            * n_sample];
    ASMTimer* timer = ASMTimer::request();
    RecordTools* stool = RecordTools::request();
    stool->setDefaultDir("results");
    Generator* gn = new Generator();
    Request* r = new Request[s_Request_Num];
    unsigned int* interval = new unsigned int[s_Request_Num];
    double r_radio = 1;
    // run the experiment under n_round different settings : TD max~ 512,4096,32768
    for(int n_round = 0; n_round < DEFAULT_MAX_NROUND; ++n_round, g_TD_Up *= 8){
        gn->setGenerator(g_BW_Down, g_BW_Up, g_TS_Down, g_TS_Up, g_TD_Down,
                g_TD_Up, g_Interval_Avg);
        unsigned int max_resource = (g_BW_Down + g_BW_Up) / 2 * g_TD_Up
                / globalRSfixedARRAY[6 + n_round] / g_Interval_Avg * r_radio;
        // set requests and the intervals
        Helper hp;
        unsigned int generatedTime = 0;
        unsigned int m_dayTime = 86400;
        double m_avgDensity = 3600 / g_Interval_Avg;
        unsigned int m_minDensity = 1;
        double m_curDensity = 0;
        for(unsigned int i = 0; i < s_Request_Num; i++){
            m_curDensity = m_avgDensity
                    + (m_avgDensity - m_minDensity)
                            * cos(
                                    (2 * M_PI * generatedTime / m_dayTime)
                                            - M_PI_2);
            interval[i] = hp.E_Rand(m_curDensity / 3600);
            generatedTime += interval[i];
            gn->getNext(&r[i]);
        }
        // initialize the storages: 0~CArray,1~CArray8,2~CIlink,3~CTlink
        unsigned int max_range = g_TS_Up + g_TD_Up;
        unsigned int index_num = max_range / g_Index_Interval;
        for(unsigned int n = 0; n < n_repeatTimes; ++n){
            unsigned int n_no = 0;
            ct[n * n_sample + n_no] = new CArrayList(max_range,
                    DEFALUT_SLOT_SIZE0);
            ct[n * n_sample + n_no]->setResourceCap(max_resource);
            n_no++;
            ct[n * n_sample + n_no] = new CArrayList(max_range,
                    DEFALUT_SLOT_SIZE1);
            ct[n * n_sample + n_no]->setResourceCap(max_resource);
            n_no++;
            ct[n * n_sample + n_no] = new CILink(index_num, max_range);
            ct[n * n_sample + n_no]->setResourceCap(max_resource);
            n_no++;
            ct[n * n_sample + n_no] = new CTLink(index_num, max_range);
            ct[n * n_sample + n_no]->setResourceCap(max_resource);
        }

        // set temporary statistics variables
        unsigned int *t_SetTime = new unsigned int[n_sample];
        memset(t_SetTime, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_Accept = new unsigned int[n_sample];
        memset(t_Accept, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_Storage = new unsigned int[n_sample];
        memset(t_Storage, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_MinSetTime = new unsigned int[n_sample];
        memset(t_MinSetTime, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_MinAccept = new unsigned int[n_sample];
        memset(t_MinAccept, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_MinStorage = new unsigned int[n_sample];
        memset(t_MinStorage, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_TSetTime = new unsigned int[n_sample];
        memset(t_TSetTime, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_TAccept = new unsigned int[n_sample];
        memset(t_TAccept, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_TStorage = new unsigned int[n_sample];
        memset(t_TStorage, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_Total = new unsigned int[n_sample];
        memset(t_Total, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_nAccept = new unsigned int[n_sample];
        memset(t_nAccept, 0, n_sample * sizeof(unsigned int));

        // (s_Request_Num+s_Interval-1)/s_Interval rounds out circle
        unsigned int outCircleNum = (s_Request_Num + s_Interval - 1)
                / s_Interval;
        unsigned int endInnerCircle = s_Interval;
        unsigned int startInnerCircle = 0;
        unsigned int curCircleNum = 0;
        unsigned int oldTime = 0;
        unsigned int curTime = 0;
        bool flag = false;
        // initial fill up phase
        if(true){
            // pre-filled up if necessary
            unsigned int n_num = n_repeatTimes * n_sample;
            unsigned int n_prefillup_num = 10000;
            if(n_prefillup_num > s_Request_Num){
                n_prefillup_num = s_Request_Num;
            }
            for(unsigned int n = 0; n < n_num; ++n){
                curTime = oldTime;
                for(unsigned int n_pfup = 0; n_pfup < n_prefillup_num;
                        ++n_pfup){
                    curTime += interval[n_pfup];
                    ct[n]->setTime(curTime);
                    flag = ct[n]->accept(r[n_pfup]);
                    if(flag){
                        ct[n]->forceInsert(r[n_pfup]);
                    }
                }
            }
            oldTime = curTime;
        }
        Sleep(5);
        for(unsigned int ocn = 0; ocn < outCircleNum; ocn++){
            // inner circle for different storage types;
            for(unsigned int n_repeat = 0; n_repeat < n_repeatTimes;
                    ++n_repeat){
                for(unsigned int n_type = 0; n_type < n_sample; ++n_type){
                    // release cpu before each loop
                    // Sleep(5);
                    curTime = oldTime;
                    curCircleNum = startInnerCircle;
                    for(; curCircleNum < endInnerCircle; curCircleNum++){
                        if(curCircleNum >= s_Request_Num){
                            cout << "ERROR!!" << endl;
                            return;
                        }
                        // using temporary variables to avoid index conculation
                        BaseAdmissionController* testSuit = ct[n_repeat
                                * n_sample + n_type];
                        Request testRequest = r[curCircleNum];
                        // run and statistics
                        curTime += interval[curCircleNum];
                        timer->start();
                        testSuit->setTime(curTime);
                        timer->end();
                        t_SetTime[n_type] += timer->getCounts();
                        timer->start();
                        flag = testSuit->accept(testRequest);
                        timer->end();
                        t_Accept[n_type] += timer->getCounts();
                        if(flag){
                            timer->start();
                            testSuit->forceInsert(testRequest);
                            timer->end();
                            t_Storage[n_type] += timer->getCounts();
                            // the accept time just add once
                            if(n_repeat == 0){
                                t_nAccept[n_type]++;
                            }
                        }
                    }
                    curCircleNum = startInnerCircle;
                    if(n_repeat != 0){
                        t_MinSetTime[n_type] =
                                t_MinSetTime[n_type] < t_SetTime[n_type] ? t_MinSetTime[n_type] :
                                        t_SetTime[n_type];
                        t_MinAccept[n_type] =
                                t_MinAccept[n_type] < t_Accept[n_type] ? t_MinAccept[n_type] :
                                        t_Accept[n_type];
                        t_MinStorage[n_type] =
                                t_MinStorage[n_type] < t_Storage[n_type] ? t_MinStorage[n_type] :
                                        t_Storage[n_type];
                    }else{
                        t_MinSetTime[n_type] = t_SetTime[n_type];
                        t_MinAccept[n_type] = t_Accept[n_type];
                        t_MinStorage[n_type] = t_Storage[n_type];
                    }
                }
                for(unsigned int n_type = 0; n_type < n_sample; ++n_type){
                    t_SetTime[n_type] = 0;
                    t_Accept[n_type] = 0;
                    t_Storage[n_type] = 0;
                }
            }
            // statistics process
            stringstream ss;
            string name;
            ss << "cost of ocn rounds-" << g_TD_Up
                    << ":n_round,limit,outCircleNum(ocn),round(endInnerCircle)";
            for(unsigned int n_type = 0; n_type < n_sample; ++n_type){
                ss << ",settime-" << n_type << ",accept-" << n_type
                        << ",storage-" << n_type << ",naccept-" << n_type
                        << ",total-" << n_type << "";
            }
            ss << ",0";
            name = ss.str();
            ss.str("");
            stool->changeName(name) << n_round << g_TD_Up << ocn
                    << endInnerCircle;
            for(unsigned int n_type = 0; n_type < n_sample; ++n_type){
                stool->get() << t_MinSetTime[n_type] << t_MinAccept[n_type]
                        << t_MinStorage[n_type] << t_nAccept[n_type]
                        << t_MinSetTime[n_type] + t_MinAccept[n_type]
                                + t_MinStorage[n_type];
            }
            stool->get() << (curTime - oldTime) / s_Interval << stool->endl;
            // set statistics variables
            for(unsigned int n_type = 0; n_type < n_sample; ++n_type){
                t_TSetTime[n_type] += t_MinSetTime[n_type];
                t_TAccept[n_type] += t_MinAccept[n_type];
                t_TStorage[n_type] += t_MinStorage[n_type];
                t_Total[n_type] = t_TSetTime[n_type] + t_TAccept[n_type]
                        + t_TStorage[n_type];
            }
            // end the loop
            oldTime = curTime;
            startInnerCircle = endInnerCircle;
            endInnerCircle =
                    endInnerCircle + s_Interval > s_Request_Num ? s_Request_Num :
                            endInnerCircle + s_Interval;
        }
        stringstream ss;
        string name;
        ss << "finally cost until n_round:n_round,g_TD_Limit";
        for(unsigned int n_type = 0; n_type < n_sample; ++n_type){
            ss << ",settime-" << n_type << ",accept-" << n_type << ",storage-"
                    << n_type << ",naccept-" << n_type << ",total-" << n_type
                    << "";
        }
        name = ss.str();
        ss.str("");
        stool->changeName(name) << n_round << g_TD_Up;
        for(unsigned int n_type = 0; n_type < n_sample; ++n_type){
            stool->get()
                    << statistics2Str(t_TSetTime[n_type],
                            (1.0 * t_Total[n_type]))
                    << statistics2Str(t_TAccept[n_type],
                            (1.0 * t_Total[n_type]))
                    << statistics2Str(t_TStorage[n_type],
                            (1.0 * t_Total[n_type]))
                    << statistics2Str(t_nAccept[n_type], (1.0 * s_Request_Num))
                    << t_Total[n_type];
        }
        cout << "AcceptRatio:" << t_nAccept[3] * 1.0 / s_Request_Num << endl;
        stool->get() << stool->endl;

        // set temporary step-by-step statistics variables
        delete[] t_SetTime;
        delete[] t_Accept;
        delete[] t_Storage;
        // min step-by-step statistics variables
        delete[] t_MinSetTime;
        delete[] t_MinAccept;
        delete[] t_MinStorage;
        // total statistics variables
        delete[] t_TSetTime;
        delete[] t_TAccept;
        delete[] t_TStorage;
        // accumulative statistics variables
        delete[] t_Total;
        delete[] t_nAccept;
        for(unsigned int i = 0; i < n_repeatTimes * n_sample; ++i){
            delete ct[i];
        }
    }
    stool->outputSeparate(filename + ".txt");
    stool->clean();
    // destruction
    delete gn;
    delete[] r;
    delete[] interval;
    delete[] ct;
}
void exMultiLinkTest(string filename) {
    // FIXMEED chenge the code
    // statistics parameters
    unsigned int s_Interval = 100;
    unsigned int s_Request_Num = 10000;
    // try to minimum sample error which is cause by the time slot of the process
    unsigned int n_repeatTimes = 5; // repeat times of each statistics;
                                    // to get the minimum cost as the final cost.
    unsigned int n_sample = 4;  // the number of samples
    unsigned int n_multiple = 1;
    unsigned int n_storageNum = 10;
    unsigned int n_prefillup_num = 10000;
    unsigned int n_TotalRequest_Num = s_Request_Num * n_multiple;
    // requests parameters
    unsigned int g_BW_Down = DEFALUT_BW_DOWN;
    unsigned int g_BW_Up = DEFALUT_BW_UP;
    unsigned int g_TS_Down = DEFALUT_TS_DOWN;
    unsigned int g_TS_Up = DEFALUT_TS_UP;
    unsigned int g_TD_Down = DEFALUT_TD_DOWN;
    unsigned int g_TD_Up = DEFALUT_TD_UP;
    unsigned int g_Interval_Avg = DEFALUT_INTERVAL_AVG;
    unsigned int g_Index_Interval = g_Interval_Avg * DEFALUT_INDEX_MULTIPLE
            * n_storageNum;
    cout << "g_Index_Interval;" << g_Index_Interval << endl;
    cout << "g_Interval_Avg;" << g_Interval_Avg << endl;
    cout << "nodesPerIndex;" << g_Index_Interval * 1.0 / g_Interval_Avg << endl;
    double d_resRadio = 0.1;
    double d_minResRadio = 0.1;
    // initialize the test units
    BaseAdmissionController** ct = new BaseAdmissionController*[n_repeatTimes
            * n_sample * n_multiple * n_storageNum];
    ASMTimer* timer = ASMTimer::request();
    RecordTools* stool = RecordTools::request();
    stool->setDefaultDir("results");
    Generator* gn = new Generator();
    Request* r = new Request[n_TotalRequest_Num];
    unsigned int* interval = new unsigned int[n_TotalRequest_Num];
    // run the experiment under n_round different settings :
    /////////////config the experiment here//////////////
    for(int n_round = 0; n_round < DEFAULT_MAX_NROUND; ++n_round, g_TD_Up *= 8){
        gn->setGenerator(g_BW_Down, g_BW_Up, g_TS_Down, g_TS_Up, g_TD_Down,
                g_TD_Up, g_Interval_Avg);
        unsigned int max_resource = (g_BW_Down + g_BW_Up) / 2 * g_TD_Up
                / globalRSfixedARRAY[9 + n_round] / g_Interval_Avg * d_resRadio;
        if(max_resource < g_BW_Up * d_minResRadio){
            max_resource = g_BW_Up * d_minResRadio;
        }
        // set requests and the intervals
        for(unsigned int i = 0; i < n_TotalRequest_Num; i++){
            interval[i] = gn->getNext(&r[i]);
        }
        // initialize the storages: 0~CArray,1~CArray8,2~CIlink,3~CTlink
        unsigned int max_range = g_TS_Up + g_TD_Up;
        unsigned int index_num = max_range / g_Index_Interval;
        for(unsigned int i_repeatTimes = 0; i_repeatTimes < n_repeatTimes;
                ++i_repeatTimes){
            for(unsigned int i_storageNum = 0; i_storageNum < n_storageNum;
                    ++i_storageNum){
                for(unsigned int i_multiple = 0; i_multiple < n_multiple;
                        ++i_multiple){
                    unsigned int i_sample = 0;
                    // {repeatTime:0{sample:0{stroge:0{multiple:0~n}~n}~n}~n}
                    ct[i_repeatTimes * n_sample * n_storageNum * n_multiple
                            + i_sample * n_storageNum * n_multiple
                            + i_storageNum * n_multiple + i_multiple] =
                            new CArrayList(max_range, DEFALUT_SLOT_SIZE0);
                    ct[i_repeatTimes * n_sample * n_storageNum * n_multiple
                            + i_sample * n_storageNum * n_multiple
                            + i_storageNum * n_multiple + i_multiple]->setResourceCap(
                            max_resource);
                    i_sample++;
                    ct[i_repeatTimes * n_sample * n_storageNum * n_multiple
                            + i_sample * n_storageNum * n_multiple
                            + i_storageNum * n_multiple + i_multiple] =
                            new CArrayList(max_range, DEFALUT_SLOT_SIZE1);
                    ct[i_repeatTimes * n_sample * n_storageNum * n_multiple
                            + i_sample * n_storageNum * n_multiple
                            + i_storageNum * n_multiple + i_multiple]->setResourceCap(
                            max_resource);
                    i_sample++;
                    ct[i_repeatTimes * n_sample * n_storageNum * n_multiple
                            + i_sample * n_storageNum * n_multiple
                            + i_storageNum * n_multiple + i_multiple] =
                            new CILink(index_num, max_range);
                    ct[i_repeatTimes * n_sample * n_storageNum * n_multiple
                            + i_sample * n_storageNum * n_multiple
                            + i_storageNum * n_multiple + i_multiple]->setResourceCap(
                            max_resource);
                    i_sample++;
                    ct[i_repeatTimes * n_sample * n_storageNum * n_multiple
                            + i_sample * n_storageNum * n_multiple
                            + i_storageNum * n_multiple + i_multiple] =
                            new CTLink(index_num, max_range);
                    ct[i_repeatTimes * n_sample * n_storageNum * n_multiple
                            + i_sample * n_storageNum * n_multiple
                            + i_storageNum * n_multiple + i_multiple]->setResourceCap(
                            max_resource);
                    i_sample++;
                }
            }
        }

        // set temporary step-by-step statistics variables
        unsigned int *t_SetTime = new unsigned int[n_sample];
        memset(t_SetTime, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_Accept = new unsigned int[n_sample];
        memset(t_Accept, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_Storage = new unsigned int[n_sample];
        memset(t_Storage, 0, n_sample * sizeof(unsigned int));
        // min step-by-step statistics variables
        unsigned int *t_MinSetTime = new unsigned int[n_sample];
        memset(t_MinSetTime, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_MinAccept = new unsigned int[n_sample];
        memset(t_MinAccept, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_MinStorage = new unsigned int[n_sample];
        memset(t_MinStorage, 0, n_sample * sizeof(unsigned int));
        // multiple step-by-step statistics variables
        unsigned int *t_MultiMinSetTime = new unsigned int[n_sample];
        memset(t_MultiMinSetTime, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_MultiMinAccept = new unsigned int[n_sample];
        memset(t_MultiMinAccept, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_MultiMinStorage = new unsigned int[n_sample];
        memset(t_MultiMinStorage, 0, n_sample * sizeof(unsigned int));
        // out-circle multiple statistics variables
        unsigned int *t_OutMultiMinSetTime = new unsigned int[n_sample];
        memset(t_OutMultiMinSetTime, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_OutMultiMinAccept = new unsigned int[n_sample];
        memset(t_OutMultiMinAccept, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_OutMultiMinStorage = new unsigned int[n_sample];
        memset(t_OutMultiMinStorage, 0, n_sample * sizeof(unsigned int));
        // total statistics variables
        unsigned int *t_TSetTime = new unsigned int[n_sample];
        memset(t_TSetTime, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_TAccept = new unsigned int[n_sample];
        memset(t_TAccept, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_TStorage = new unsigned int[n_sample];
        memset(t_TStorage, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_Total = new unsigned int[n_sample];
        // accumulative statistics variables
        memset(t_Total, 0, n_sample * sizeof(unsigned int));
        unsigned int *t_nAccept = new unsigned int[n_sample];
        memset(t_nAccept, 0, n_sample * sizeof(unsigned int));

        // (s_Request_Num+s_Interval-1)/s_Interval rounds out circle
        unsigned int outCircleNum = (s_Request_Num + s_Interval - 1)
                / s_Interval;
        unsigned int endInnerCircle = s_Interval;
        unsigned int startInnerCircle = 0;
        unsigned int curCircleNum = 0;
        unsigned int oldTime = 0;
        unsigned int curTime = 0;
        bool flag = false;
        // initial fill up phase
        if(true){
            // FIXMEED the pre-filled up process is different in multilink test
            // pre-filled up if necessary
            if(n_prefillup_num > s_Request_Num){
                n_prefillup_num = s_Request_Num;
            }
            for(unsigned int i_prefillup_num = 0;
                    i_prefillup_num < n_prefillup_num; ++i_prefillup_num){
                for(unsigned int i_multiple = 0; i_multiple < n_multiple;
                        ++i_multiple){
                    // using different requests sets
                    for(unsigned int i_sample = 0; i_sample < n_sample;
                            ++i_sample){
                        // test different storage types
                        for(unsigned int i_repeatTimes = 0;
                                i_repeatTimes < n_repeatTimes; ++i_repeatTimes){
                            // run several times to get the min cost to avoid the error causing by cpu slot of system
                            curTime = oldTime;
                            // using same interval in each one of multiple
                            curTime += interval[i_prefillup_num];
                            // reset accept flag before each repeat
                            bool f_hasBeenAccepted = false;
                            for(unsigned int i_storageNum = 0;
                                    i_storageNum < n_storageNum
                                            && !f_hasBeenAccepted;
                                    ++i_storageNum){
                                // multiple links
                                // using temporary variables to avoid index conculation
                                BaseAdmissionController* testSuit =
                                        ct[i_repeatTimes * n_sample
                                                * n_storageNum * n_multiple
                                                + i_sample * n_storageNum
                                                        * n_multiple
                                                + i_storageNum * n_multiple
                                                + i_multiple];
                                Request testRequest = r[i_prefillup_num
                                        * n_multiple + i_multiple];
                                testSuit->setTime(curTime);
                                flag = testSuit->accept(testRequest);
                                if(flag){
                                    f_hasBeenAccepted = true;
                                    testSuit->forceInsert(testRequest);
                                }
                            }
                        }
                    }
                }
                oldTime = curTime;
            }
        }
        for(unsigned int ocn = 0; ocn < outCircleNum; ocn++){
            // inner circle for different storage types;
            // for each circle, run multiple times with different
            // requests: request[i_Request_Num*s_Request_Num+i_multiple]
            // to get the average cost in average situation.
            // release cpu before each loop
            // Sleep(5);
            curCircleNum = startInnerCircle;
            for(; curCircleNum < endInnerCircle; curCircleNum++){
                if(curCircleNum >= s_Request_Num){
                    cout << "ERROR!!" << endl;
                    return;
                }
                for(unsigned int i_multiple = 0; i_multiple < n_multiple;
                        ++i_multiple){
                    // using different requests sets
                    for(unsigned int i_sample = 0; i_sample < n_sample;
                            ++i_sample){
                        // test different storage types
                        for(unsigned int i_repeatTimes = 0;
                                i_repeatTimes < n_repeatTimes; ++i_repeatTimes){
                            // run several times to get the min cost to avoid the error causing by cpu slot of system
                            curTime = oldTime;
                            // using same interval in each one of multiple
                            curTime += interval[curCircleNum];
                            // reset accept flag before each repeat
                            bool f_hasBeenAccepted = false;
                            for(unsigned int i_storageNum = 0;
                                    i_storageNum < n_storageNum
                                            && !f_hasBeenAccepted;
                                    ++i_storageNum){
                                // multiple links
                                // using temporary variables to avoid index conculation
                                BaseAdmissionController* testSuit =
                                        ct[i_repeatTimes * n_sample
                                                * n_storageNum * n_multiple
                                                + i_sample * n_storageNum
                                                        * n_multiple
                                                + i_storageNum * n_multiple
                                                + i_multiple];
                                Request testRequest = r[curCircleNum
                                        * n_multiple + i_multiple];
                                timer->start();
                                testSuit->setTime(curTime);
                                timer->end();
                                t_SetTime[i_sample] += timer->getCounts();
                                timer->start();
                                flag = testSuit->accept(testRequest);
                                timer->end();
                                t_Accept[i_sample] += timer->getCounts();
                                if(flag){
                                    f_hasBeenAccepted = true;
                                    timer->start();
                                    testSuit->forceInsert(testRequest);
                                    timer->end();
                                    t_Storage[i_sample] += timer->getCounts();
                                    // the accept time just add once
                                    if(i_repeatTimes == 0){
                                        cout
                                                << "multilink accepted storage No.: "
                                                << i_storageNum << endl;
                                        cout << "r.td:r.bw\t" << testRequest.duration
                                                << ":" << testRequest.value
                                                << endl;
                                        t_nAccept[i_sample]++;
                                    }
                                }
                            }
                            if(i_repeatTimes == 0){
                                t_MinSetTime[i_sample] = t_SetTime[i_sample];
                                t_MinAccept[i_sample] = t_Accept[i_sample];
                                t_MinStorage[i_sample] = t_Storage[i_sample];
                            }else{
                                t_MinSetTime[i_sample] =
                                        t_MinSetTime[i_sample]
                                                < t_SetTime[i_sample] ? t_MinSetTime[i_sample] :
                                                t_SetTime[i_sample];
                                t_MinAccept[i_sample] =
                                        t_MinAccept[i_sample]
                                                < t_Accept[i_sample] ? t_MinAccept[i_sample] :
                                                t_Accept[i_sample];
                                t_MinStorage[i_sample] =
                                        t_MinStorage[i_sample]
                                                < t_Storage[i_sample] ? t_MinStorage[i_sample] :
                                                t_Storage[i_sample];
                            }
                            t_SetTime[i_sample] = 0;
                            t_Accept[i_sample] = 0;
                            t_Storage[i_sample] = 0;
                        }
                        if(i_multiple == 0){
                            t_MultiMinSetTime[i_sample] =
                                    t_MinSetTime[i_sample];
                            t_MultiMinAccept[i_sample] = t_MinAccept[i_sample];
                            t_MultiMinStorage[i_sample] =
                                    t_MinStorage[i_sample];
                        }else{
                            t_MultiMinSetTime[i_sample] +=
                                    t_MinSetTime[i_sample];
                            t_MultiMinAccept[i_sample] += t_MinAccept[i_sample];
                            t_MultiMinStorage[i_sample] +=
                                    t_MinStorage[i_sample];
                        }
                    }
                }
                oldTime = curTime;
                for(unsigned int i_sample = 0; i_sample < n_sample; ++i_sample){
                    t_MultiMinSetTime[i_sample] = t_MultiMinSetTime[i_sample]
                            / n_multiple;
                    t_MultiMinAccept[i_sample] = t_MultiMinAccept[i_sample]
                            / n_multiple;
                    t_MultiMinStorage[i_sample] = t_MultiMinStorage[i_sample]
                            / n_multiple;
                    // outMultiMinXXX: the total average MinXXX time of one total inner circle.
                    t_OutMultiMinSetTime[i_sample] +=
                            t_MultiMinSetTime[i_sample];
                    t_OutMultiMinAccept[i_sample] += t_MultiMinAccept[i_sample];
                    t_OutMultiMinStorage[i_sample] +=
                            t_MultiMinStorage[i_sample];
                }
            }
            // statistics process
            // set statistics variables
            for(unsigned int i_sample = 0; i_sample < n_sample; ++i_sample){
                // outMultiMinXXX: the total average MinXXX time of one total inner circle.
                // TXXX: the finaly total time cost.
                t_TSetTime[i_sample] += t_OutMultiMinSetTime[i_sample];
                t_TAccept[i_sample] += t_OutMultiMinAccept[i_sample];
                t_TStorage[i_sample] += t_OutMultiMinStorage[i_sample];
                t_Total[i_sample] = t_TSetTime[i_sample] + t_TAccept[i_sample]
                        + t_TStorage[i_sample];
            }
            stringstream ss;
            string name;
            ss << "cost of ocn rounds-" << g_TD_Up
                    << ":n_round,limit,outCircleNum(ocn),round(endInnerCircle)";
            for(unsigned int n_type = 0; n_type < n_sample; ++n_type){
                ss << ",settime-" << n_type << ",accept-" << n_type
                        << ",storage-" << n_type << ",naccept-" << n_type
                        << ",total-" << n_type << "";
            }
            name = ss.str();
            ss.str("");
            stool->changeName(name) << n_round << g_TD_Up << ocn
                    << endInnerCircle;
            for(unsigned int i_sample = 0; i_sample < n_sample; ++i_sample){
                // outMultiMinXXX: the total average MinXXX time of one total inner circle.
                // nAccept: the total accepted requests; nAccept/multiple: the average accepted requests for one multiple.
                // at last, reset outMultiMinXXX.
                stool->get() << t_OutMultiMinSetTime[i_sample]
                        << t_OutMultiMinAccept[i_sample]
                        << t_OutMultiMinStorage[i_sample]
                        << t_nAccept[i_sample] / n_multiple
                        << t_OutMultiMinSetTime[i_sample]
                                + t_OutMultiMinAccept[i_sample]
                                + t_OutMultiMinStorage[i_sample];
                t_OutMultiMinSetTime[i_sample] = 0;
                t_OutMultiMinAccept[i_sample] = 0;
                t_OutMultiMinStorage[i_sample] = 0;
            }
            stool->get() << stool->endl;
            // end the loop
            startInnerCircle = endInnerCircle;
            endInnerCircle =
                    endInnerCircle + s_Interval > s_Request_Num ? s_Request_Num :
                            endInnerCircle + s_Interval;
        }
        stringstream ss;
        string name;
        ss << "finally cost until n_round:n_round,g_TD_Limit";
        for(unsigned int n_type = 0; n_type < n_sample; ++n_type){
            ss << ",settime-" << n_type << ",accept-" << n_type << ",storage-"
                    << n_type << ",naccept-" << n_type << ",total-" << n_type
                    << "";
        }
        name = ss.str();
        ss.str("");
        stool->changeName(name) << n_round << g_TD_Up;
        for(unsigned int n_type = 0; n_type < n_sample; ++n_type){
            stool->get()
                    << statistics2Str(t_TSetTime[n_type],
                            (1.0 * t_Total[n_type]))
                    << statistics2Str(t_TAccept[n_type],
                            (1.0 * t_Total[n_type]))
                    << statistics2Str(t_TStorage[n_type],
                            (1.0 * t_Total[n_type]))
                    << statistics2Str(t_nAccept[n_type], (1.0 * s_Request_Num))
                    << t_Total[n_type];
        }
        cout << "AcceptRatio:" << t_nAccept[3] * 1.0 / s_Request_Num << endl;
        stool->get() << stool->endl;
        // destruction
        // set temporary step-by-step statistics variables
        delete[] t_SetTime;
        delete[] t_Accept;
        delete[] t_Storage;
        // min step-by-step statistics variables
        delete[] t_MinSetTime;
        delete[] t_MinAccept;
        delete[] t_MinStorage;
        // multiple step-by-step statistics variables
        delete[] t_MultiMinSetTime;
        delete[] t_MultiMinAccept;
        delete[] t_MultiMinStorage;
        // out-circle multiple statistics variables
        delete[] t_OutMultiMinSetTime;
        delete[] t_OutMultiMinAccept;
        delete[] t_OutMultiMinStorage;
        // total statistics variables
        delete[] t_TSetTime;
        delete[] t_TAccept;
        delete[] t_TStorage;
        // accumulative statistics variables
        delete[] t_Total;
        delete[] t_nAccept;
        for(unsigned int i = 0;
                i < n_repeatTimes * n_sample * n_multiple * n_storageNum; ++i){
            delete ct[i];
        }
    }
    stool->outputSeparate(filename + ".txt");
    stool->clean();
    // destruction
    delete gn;
    delete[] r;
    delete[] interval;
    delete[] ct;
}
int main() {
    enum ExperimentCode {
        EX_ASM_TEST = 0,
        EX_HELPER_TEST,
        EX_DEVELOP_TEST,
        EX_COMMON_TEST,
        EX_START_PHASE,
        EX_UNBLANCE,
        EX_MULTILINK,
        EX_LAST_FLAG
    };
    string flagStrArray[] = {
            "EX_ASM_TEST",
            "EX_HELPER_TEST",
            "EX_DEVELOP_TEST",
            "EX_COMMON_TEST",
            "EX_START_PHASE",
            "EX_UNBLANCE",
            "EX_MULTILINK",
            "EX_LAST_FLAG" };
    bool *flagArray = new bool[EX_LAST_FLAG];
    {
        int flag = 0;
        // cout << "Input flag for each experiment: 1 for run, 0 for not." << endl;
        for(int i = 0; i < EX_LAST_FLAG; ++i){
            cout << flagStrArray[i] << endl;
            //cin >> flag;
            flag = 1;
            if(flag == 1){
                flagArray[i] = true;
            }else if(flag == 0){
                flagArray[i] = false;
            }
        }
    }

    double a = 12.32124214;
    cout << dbl2Str(a, 2) << endl;
    // for test
    bool b_run = true;
    string prefixStr = "./results/";
    if(false && !flagArray[EX_ASM_TEST]){
        exASMTest();
    }
    if(false && !flagArray[EX_HELPER_TEST]){
        exHelperTest();
    }
    if(false && !flagArray[EX_DEVELOP_TEST]){
        exDevelopTest();
    }
    if(b_run && flagArray[EX_COMMON_TEST]){
        exCommonTest(flagStrArray[EX_COMMON_TEST]);
    }
    if(false && flagArray[EX_START_PHASE]){
        exStartPhaseTest(flagStrArray[EX_START_PHASE]);
    }
    if(false && flagArray[EX_UNBLANCE]){
        exUnlanceTest(flagStrArray[EX_UNBLANCE]);
    }
    if(false && flagArray[EX_MULTILINK]){
        exMultiLinkTest(flagStrArray[EX_MULTILINK]);
    }
    // 2.25,8,16,1,2.75,0.225
    if(!b_run){
        for(double var = 0; var < 0; var += 0.25){
            globalRSfixedARRAY[0] = var;    // 2.25
            globalRSfixedARRAY[1] = var;    // 8
            globalRSfixedARRAY[2] = var;    // 16
            cout << "EX_COMMON_TEST:" << var << endl;
            exCommonTest(flagStrArray[EX_COMMON_TEST]);
        }
        for(double var = 0; var < 0; ++var){
            globalRSfixedARRAY[3] = var;    // 1
            globalRSfixedARRAY[4] = var;    // 1
            globalRSfixedARRAY[5] = var;    // 1
            cout << "EX_START_PHASE:" << var << endl;
            exStartPhaseTest(flagStrArray[EX_START_PHASE]);
        }
        for(double var = 19; var < 20; var += 0.1){
            globalRSfixedARRAY[6] = var;    // 2.75
            globalRSfixedARRAY[7] = var;    // 10
            globalRSfixedARRAY[8] = var;    // 19
            cout << "EX_UNBLANCE:" << var << endl;
            exUnlanceTest(flagStrArray[EX_UNBLANCE]);
        }
        for(double var = 19; var < 20; var += 0.1){
            globalRSfixedARRAY[9] = var;  // 0.43
            globalRSfixedARRAY[10] = var;  // 3.4
            globalRSfixedARRAY[11] = var;  // 19
            cout << "EX_MULTILINK:" << var << endl;
            exMultiLinkTest(flagStrArray[EX_MULTILINK]);
        }
    }
}

