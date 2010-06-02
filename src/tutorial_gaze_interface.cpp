// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
//
// A tutorial on how to use the Gaze Interface.
//
// Author: Ugo Pattacini - <ugo.pattacini@iit.it>

#include <yarp/os/Network.h>
#include <yarp/os/RFModule.h>
#include <yarp/os/RateThread.h>
#include <yarp/os/Time.h>
#include <yarp/sig/Vector.h>
#include <yarp/math/Math.h>

#include <yarp/dev/Drivers.h>
#include <yarp/dev/ControlBoardInterfaces.h>
#include <yarp/dev/GazeControl.h>
#include <yarp/dev/PolyDriver.h>

#include <gsl/gsl_math.h>

#include <stdio.h>
#include <deque>

#define CTRL_THREAD_PER     0.02        // [s]
#define PRINT_STATUS_PER    1.0         // [s]
#define STORE_POI_PER       3.0         // [s]
#define SWITCH_STATE_PER    10.0        // [s]

#define STATE_TRACK         0
#define STATE_RECALL        1
#define STATE_WAIT          2

YARP_DECLARE_DEVICES(icubmod)

using namespace std;
using namespace yarp;
using namespace yarp::os;
using namespace yarp::dev;
using namespace yarp::sig;
using namespace yarp::math;



class CtrlThread: public RateThread
{
protected:
    PolyDriver   *client;
    IGazeControl *gaze;

    int state;

    Vector fp;

    deque<Vector> poiList;

    double t;
    double t0;
    double t1;
    double t2;
    double t3;

public:
    CtrlThread(const double period) : RateThread(int(period*1000.0)) { }

    virtual bool threadInit()
    {
        // open a client interface to connect to the gaze server
        // we suppose that:
        // 1 - the iCub simulator (icubSim) is running
        // 2 - the gaze server iKinGazeCtrl is running and
        //     launched with --robot icubSim option
        Property option("(device gazecontrollerclient)");
        option.put("remote","/iKinGazeCtrl");
        option.put("local","/gaze_client");

        client=new PolyDriver;
        if (!client->open(option))
        {
            delete client;    
            return false;
        }

        // open the view
        client->view(gaze);

        // set trajectory time
        gaze->setNeckTrajTime(0.4);
        gaze->setEyesTrajTime(0.1);

        fp.resize(3);

        state=STATE_TRACK;

        return true;
    }

    virtual void afterStart(bool s)
    {
        if (s)
            fprintf(stdout,"Thread started successfully\n");
        else
            fprintf(stdout,"Thread did not start\n");

        t=t0=t1=t2=t3=Time::now();
    }

    virtual void run()
    {
        t=Time::now();

        generateTarget();        

        if (state==STATE_TRACK)
        {
            // look at the target
            // in streaming
            gaze->lookAtFixationPoint(fp);

            // some verbosity
            printStatus();

            // we collect from time to time
            // some interesting points (POI)
            // where to look at soon afterwards
            storeInterestingPoint();

            if (t-t2>=SWITCH_STATE_PER)
            {
                // switch state
                state=STATE_RECALL;
            }
        }

        if (state==STATE_RECALL)
        {
            // pick up the first POI
            // and clear the list
            Vector ang=poiList.front();
            poiList.clear();

            fprintf(stdout,"Retrieving POI #0 ... %s [deg]\n",
                    ang.toString().c_str());

            // look at the chosen POI
            gaze->lookAtAbsAngles(ang);

            // switch state
            state=STATE_WAIT;
        }

        if (state==STATE_WAIT)
        {
            bool done=false;
            gaze->checkMotionDone(&done);

            if (done)
            {
                Vector ang;
                gaze->getAngles(ang);            

                fprintf(stdout,"Actual gaze configuration: %s [deg]\n",
                        ang.toString().c_str());

                t1=t2=t3=t;
    
                // switch state
                state=STATE_TRACK;
            }
        }
    }

    virtual void threadRelease()
    {    
        // we require an immediate stop
        // before closing the client for safety reason
        // (anyway it's already done internally in the
        // destructor)
        gaze->stopControl();
        delete client;
    }

    void generateTarget()
    {   
        // translational target part: a circular trajectory
        // in the yz plane centered in [-0.5,0.0,0.25] with radius=0.1 m
        // and frequency 0.1 Hz
        fp[0]=-0.50;
        fp[1]=+0.00+0.1*cos(2.0*M_PI*0.1*(t-t0));
        fp[2]=+0.25+0.1*sin(2.0*M_PI*0.1*(t-t0));            
    }

    void storeInterestingPoint()
    {
        if (t-t3>=STORE_POI_PER)
        {
            Vector ang;

            // we store the current azimuth, elevation
            // and vergence wrt the absolute reference frame
            // The absolute reference frame for the azimuth/elevation couple
            // is head-centered with the robot in rest configuration
            // (i.e. torso and head angles zeroed). 
            gaze->getAngles(ang);            

            fprintf(stdout,"Storing POI #%d ... %s [deg]\n",
                    poiList.size(),ang.toString().c_str());

            poiList.push_back(ang);

            t3=t;
        }
    }

    double norm(const Vector &v)
    {
        return sqrt(dot(v,v));
    }

    void printStatus()
    {        
        if (t-t1>=PRINT_STATUS_PER)
        {
            Vector x;

            // we get the current fixation point in the
            // operational space
            gaze->getFixationPoint(x);

            fprintf(stdout,"+++++++++\n");
            fprintf(stdout,"fp         [m] = %s\n",fp.toString().c_str());
            fprintf(stdout,"x          [m] = %s\n",x.toString().c_str());
            fprintf(stdout,"norm(fp-x) [m] = %g\n",norm(fp-x));
            fprintf(stdout,"---------\n\n");

            t1=t;
        }
    }
};



class CtrlModule: public RFModule
{
protected:
    CtrlThread *thr;

public:
    virtual bool configure(ResourceFinder &rf)
    {
        Time::turboBoost();

        thr=new CtrlThread(CTRL_THREAD_PER);
        if (!thr->start())
        {
            delete thr;
            return false;
        }

        return true;
    }

    virtual bool close()
    {
        thr->stop();
        delete thr;

        return true;
    }

    virtual double getPeriod()    { return 1.0;  }
    virtual bool   updateModule() { return true; }
};



int main(int argc, char *argv[])
{
    // we need to initialize the drivers list 
    YARP_REGISTER_DEVICES(icubmod)

    Network yarp;
    if (!yarp.checkNetwork())
        return -1;

    CtrlModule mod;

    ResourceFinder rf;
    return mod.runModule(rf);
}



