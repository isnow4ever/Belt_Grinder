/*
* License:The MIT License (MIT)
*
* Copyright (c) 2018 Yongzhuo Gao
* State Key Laboratory of Robotics and System, Harbin Institute of Technology
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*/

#include <belt_grinder/belt_grinder.h>
#include <ethercat_manager/ethercat_manager.h>
#include <minas_control/minas_client.h>

void timespecInc(struct timespec &tick, int nsec)
{
  tick.tv_nsec += nsec;
  while (tick.tv_nsec >= NSEC_PER_SECOND)
    {
      tick.tv_nsec -= NSEC_PER_SECOND;
      tick.tv_sec++;
    }
}

int main(int argc, char *argv[])
{
    // initial ATI Mini45 force sensor
    std::string ip = "192.168.1.1";
    ati_sensor f_sensor(ip); 
    
    // initial Minas Driver
    std::string ifname = "eth0";
    ethercat::EtherCatManager manager(ifname);

    minas_control::MinasClient *client = new minas_control::MinasClient(manager, 1);
    // clear error
    client->reset();

    // set paramete from PANATERM test program
    client->setTrqueForEmergencyStop(100); // 100%
    client->setOverLoadLevel(50);          // 50%
    client->setOverSpeedLevel(120);        // r/min
    client->setMotorWorkingRange(0.1);     // 0.1

    client->setInterpolationTimePeriod(4000);     // 4 msec

    // servo on
    client->servoOn();
    // get current positoin
    minas_control::MinasInput input = client->readInputs();
    int32 current_position = input.position_actual_value;
    // set target position
    minas_control::MinasOutput output;
    memset(&output, 0x00, sizeof(minas_control::MinasOutput));
      
    output.target_position = current_position;

    output.max_motor_speed = 120;  // rad/min
    output.target_torque = 500;    // 0% (unit 0.1%)
    output.max_torque    = 500;    // 50% (unit 0.1%)
    output.controlword   = 0x001f; // move to operation enabled + new-set-point (bit4) + change set immediately (bit5)
    
    int operation_mode = 0x08; // (csp) cyclic synchronous position mode
    // change to cyclic synchronous position mode
    output.operation_mode = operation_mode;

    // set profile velocity
    client->setProfileVelocity(0x20000000);  
    // while ( ! (input.statusword & 0x1000) )
    // {// bit12 (set-point-acknowledge)Ϊ1����ѭ�� 
    //     cout<<"statusword:"<<input.statusword<<endl;
	    
    // }
    input = client->readInputs();
    output.controlword   &= ~0x0010; // clear new-set-point (bit4)
    client->writeOutputs(output);

    int b;
    bool thread_flag = false;
    boost::thread thrd;
    cout<<"Mini45 calibration start!"<<endl;
    sleep(3);
    f_sensor.zero();
    cout<<"Mini45 zero calibration finished!"<<endl;
    cout<<"sleep for 3s."<<endl;
    sleep(3);

    thrd = boost::thread(boost::bind(&ati_sensor::read_all, &f_sensor));
    thread_flag = true;
    cout<<"thread on!"<<endl;


Loop:
    cout<<endl<<"Please choose which mini45 will do."<<endl<<"1:Zero,2:Read once,3:Start/Stop read continually,4:Current value"<<endl;
    //cout<<"Please imput the number."<<endl;
    cin>>b;

    if(b==1)
    {
        f_sensor.zero();
        goto Loop;
    }
    else if(b==2)
    {
        f_sensor.read_once();
        goto Loop;
    }
    else if(b==3)
    {
        if(!thread_flag)
        {
            thrd = boost::thread(boost::bind(&ati_sensor::read_all, &f_sensor));
            thread_flag = true;
            cout<<"thread on"<<endl;
        }            
        else
        {
            thrd.interrupt();
            //thrd.join();
            thread_flag = false;
            cout<<"thread off"<<endl;
        }

        goto Loop;
    }
    else if(b==4)
    {
        // 4ms in nanoseconds
        double period = 4e+6;
        // get curren ttime
        struct timespec tick;
        clock_gettime(CLOCK_REALTIME, &tick);
        timespecInc(tick, period);

        
        for(int counts = 0; counts <= 10000;counts++) 
        {
        
            minas_control::MinasInput input = client->readInputs();
            minas_control::MinasOutput output = client->readOutputs();
            if ( counts % 100 == 0)
            {
                printf("err = %04x, ctrl %04x, status %04x, op_mode = %2d, pos = %08x, vel = %08x, tor = %08x\n",
                input.error_code, output.controlword, input.statusword, input.operation_mode, input.position_actual_value, input.velocity_actual_value, input.torque_actual_value);
                if ( input.statusword & 0x0400 ) { // target reached (bit 10)
                printf("target reached\n");
                break;
                }
                printf("Tick %8ld.%09ld\n", tick.tv_sec, tick.tv_nsec);
                printf("Input:\n");
                printf(" 603Fh %08x :Error code\n", input.error_code);
                printf(" 6041h %08x :Statusword\n", input.statusword);
                printf(" 6061h %08x :Modes of operation display\n", input.operation_mode);
                printf(" 6064h %08x :Position actual value\n", input.position_actual_value);
                printf(" 606Ch %08x :Velocity actual value\n", input.velocity_actual_value);
                printf(" 6077h %08x :Torque actual value\n", input.torque_actual_value);
                printf(" 60B9h %08x :Touch probe status\n", input.touch_probe_status);
                printf(" 60BAh %08x :Touch probe pos1 pos value\n", input.touch_probe_posl_pos_value);
                printf(" 60FDh %08x :Digital inputs\n", input.digital_inputs);
                printf("Output:\n");
                printf(" 6040h %08x :Controlword\n", output.controlword);
                printf(" 6060h %08x :Mode of operation\n", output.operation_mode);
                printf(" 6071h %08x :Target Torque\n", output.target_torque);
                printf(" 6072h %08x :Max Torque\n", output.max_torque);
                printf(" 607Ah %08x :Target Position\n", output.target_position);
                printf(" 6080h %08x :Max motor speed\n", output.max_motor_speed);
                printf(" 60B8h %08x :Touch Probe function\n", output.touch_probe_function);
                printf(" 60FFh %08x :Target Velocity\n", output.target_velocity);
                printf(" 60B0h %08x :Position Offset\n", output.position_offset);

                for(int i=0;i<6;++i)
                cout<<f_sensor.resp.FTData[i]/1000000<<" , ";
                cout<<endl;
            }

            //output.controlword   |= 0x0004; // enable new-set-point (bit4)
            // (csp) cyclic synchronous position mode
            double force = double(f_sensor.resp.FTData[1])/1000000;
            double sensitivity;
            if(abs(force) >= 20)
                sensitivity = 2*(force/abs(force));
            else
                sensitivity = force / 10;

            if(abs(force) >= 2)
            {
                current_position = input.position_actual_value;
                output.target_position = current_position;
                output.position_offset = -0x002500*sensitivity;
            }

            client->writeOutputs(output);
        

            //usleep(4*1000);
            timespecInc(tick, period);
            // check overrun
            struct timespec before;
            clock_gettime(CLOCK_REALTIME, &before);
            double overrun_time = (before.tv_sec + double(before.tv_nsec)/NSEC_PER_SECOND) -  (tick.tv_sec + double(tick.tv_nsec)/NSEC_PER_SECOND);
            if (overrun_time > 0.0)
            {
                fprintf(stderr, "  overrun: %f", overrun_time);
            }
            clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &tick, NULL);
        }

    
        minas_control::MinasInput input = client->readInputs();
        client->printPDSStatus(input);
        client->printPDSOperation(input);
        client->servoOff();


        goto Loop;
    }
    else
    {
        cout<<"Please point 1,2,3or4."<<endl;
        goto Loop;
    }

    return 0;
       
}