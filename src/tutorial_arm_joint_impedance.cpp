// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

#include <stdio.h>
#include <yarp/os/Network.h>
#include <yarp/dev/ControlBoardInterfaces.h>
#include <yarp/dev/PolyDriver.h>
#include <yarp/os/Time.h>
#include <yarp/sig/Vector.h>

#include <string>

using namespace yarp::dev;
using namespace yarp::sig;
using namespace yarp::os;
using namespace yarp;

int main(int argc, char *argv[]) 
{
    Network yarp;

    Property params;
    params.fromCommand(argc, argv);

    if (!params.check("robot"))
    {
        fprintf(stderr, "Please specify the name of the robot\n");
        fprintf(stderr, "--robot name (e.g. icub)\n");
        return -1;
    }
    std::string robotName=params.find("robot").asString().c_str();
    std::string remotePorts="/";
    remotePorts+=robotName;
    remotePorts+="/right_arm";

    std::string localPorts="/test/client";

    Property options;
    options.put("device", "remote_controlboard");
    options.put("local", localPorts.c_str());   //local port names
    options.put("remote", remotePorts.c_str());         //where we connect to

    // create a device
    PolyDriver robotDevice(options);
    if (!robotDevice.isValid()) {
        printf("Device not available.  Here are the known devices:\n");
        printf("%s", Drivers::factory().toString().c_str());
        return 0;
    }

    IPositionControl *pos;
    IEncoders *encs;
	IControlMode *ictrl;
	IImpedanceControl *iimp;
	ITorqueControl *itrq;


    bool ok;
    ok = robotDevice.view(pos);
    ok = ok && robotDevice.view(encs);
	ok = ok && robotDevice.view(ictrl);
	ok = ok && robotDevice.view(iimp);
	ok = ok && robotDevice.view(itrq);

    if (!ok) {
        printf("Problems acquiring interfaces\n");
        return 0;
    }

    int nj=0;
    pos->getAxes(&nj);
    Vector encoders;
	Vector torques;
    Vector command;
    Vector tmp;
	int control_mode;
    encoders.resize(nj);
	torques.resize(nj);
    tmp.resize(nj);
    command.resize(nj);
    
    int i;
    for (i = 0; i < nj; i++) {
         tmp[i] = 50.0;
    }
    pos->setRefAccelerations(tmp.data());

    for (i = 0; i < nj; i++) {
        tmp[i] = 10.0;
        pos->setRefSpeed(i, tmp[i]);
    }

    //pos->setRefSpeeds(tmp.data()))
    
    //fisrst zero all joints
    //
    command=0;
    //now set the shoulder to some value
    command[0]=50;
    command[1]=20;
    command[2]=-10;
    command[3]=50;
    pos->positionMove(command.data());
    
    bool done=false;

    while(!done)
        {
            pos->checkMotionDone(&done);
            Time::delay(0.1);
        }

    int times=0;
    while(true)
    {
        times++;
        if (times%2)
        {
             command[0]=50;
             command[1]=20;
             command[2]=-10;
             command[3]=50;
        }
        else
        {
             command[0]=20;
             command[1]=10;
             command[2]=-10;
             command[3]=30;
        }

        pos->positionMove(command.data());

        int count=50;
        while(count--)
            {
                Time::delay(0.1);
                encs->getEncoders(encoders.data());
				itrq->getTorques(torques.data());
				printf("Encoders: %.1lf %.1lf %.1lf %.1lf", encoders[0], encoders[1], encoders[2], encoders[3]);
				printf("Torques:  %.1lf %.1lf %.1lf %.1lf", torques[0], torques[1], torques[2], torques[3]);
				printf("Control:  ");
				for (i = 0; i < 4; i++)
				{
					ictrl->getControlMode(i, &control_mode);
					switch (control_mode)
					{
						case VOCAB_CM_IDLE:			printf("IDLE     ");	break;
						case VOCAB_CM_POSITION:		printf("POS      ");	break;
						case VOCAB_CM_VELOCITY:		printf("VEL      ");	break;
						case VOCAB_CM_TORQUE:		printf("TORQUE   ");	break;
						case VOCAB_CM_IMPEDANCE_POS:printf("IMPED POS");	break;
						case VOCAB_CM_IMPEDANCE_VEL:printf("IMPED VEL");	break;
						case VOCAB_CM_OPENLOOP:		printf("OPENLOOP ");	break;
						default:
						case VOCAB_CM_UNKNOWN:		printf("UNKNOWN  ");	break;
					}
				}
            }
    }

    robotDevice.close();
    
    return 0;
}
