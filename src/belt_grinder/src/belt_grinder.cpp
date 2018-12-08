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

int main(int argc, char *argv[])
{
    std::string ip = "192.168.1.1";
    ati_sensor f_sensor(ip);
    
    int b;
Loop:
    cout<<endl<<"Please choose which mini45 will do."<<endl<<"1:Zero,2:Read once,3:Read continually,4:Filter"<<endl;
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
        f_sensor.of.open("data.txt");
        if(!f_sensor.of)
            cout<<"File was created failed!"<<endl;
        
        boost::thread thrd = boost::thread(boost::bind(&ati_sensor::read_all, &f_sensor));
        //f_sensor.set_timer();
        //while(1);
        f_sensor.of.close();
        goto Loop;
    }
    else if(b==4)
    {
        boost::thread thrd = boost::thread(boost::bind(&ati_sensor::filter, &f_sensor));
        //f_sensor.set_timer();
        while(1);
    }
    else
    {
        cout<<"Please point 1,2,3or4."<<endl;
        goto Loop;
    }

    return 0;
       
}