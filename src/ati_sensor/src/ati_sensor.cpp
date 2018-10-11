#include <ati_sensor/ati_sensor.h>


ATI_sensor::ATI_sensor(std::string& ipaddress)
{
    //char ipaddress[35] = "192.168.1.1";
    sockaddr_in cli;
    cli.sin_addr.s_addr = inet_addr(ipaddress);
    cli.sin_family = AF_INET;
    cli.sin_port = htons(49152);

    clisock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);//创建socket
    if (clisock == -1)
        cout<<"Socket could not be opened"<<endl;
    else
        cout<<"Socket is opened"<<endl;
    if (connect(clisock, (sockaddr*)&(cli), sizeof(cli))==0)//连接到服务器
        cout<<"Connect successfully"<<endl;
    else
        cout<<"Connection failed"<<endl;

    *(uint16*)&request[0] = htons(0x1234); /* standard header. */
    *(uint16*)&request[2] = htons(2); /* per table 9.1 in Net F/T user manual. */
    *(uint32*)&request[4] = htonl(1); /* see section 9.1 in Net F/T user manual. */

    respSource = 5;
    period_usec = 30000;
    counts_totle = 100000;
}

ATI_sensor::~ATI_sensor()
{
}

void ATI_sensor::set_timer()
{
    struct itimerval itv, oldtv;
    itv.it_interval.tv_sec = 0;
    itv.it_interval.tv_usec = period_usec;
    itv.it_value.tv_sec = 0;
    itv.it_value.tv_usec = period_usec;
    counts=0;
    clock_gettime(CLOCK_REALTIME, &before);
    begin_time=before;
    setitimer(ITIMER_REAL, &itv, &oldtv);
}

void ATI_sensor::uninit_time()
{
    struct itimerval value;
    value.it_value.tv_sec = 0;
    value.it_value.tv_usec = 0;
    value.it_interval = value.it_value;
    setitimer(ITIMER_REAL, &value, NULL);
}

void ATI_sensor::read_once()
{
    send(clisock, (const char*)request, 8, 0); //发送请求，接受数据
    recv(clisock, (char*)response, 36, 0);
    for (int i = 0; i < 6; i++)
            resp.FTData[i] = ntohl(*(int32*)&response[12 + i * 4])- resp.FTZero[i];
    int32 temp_FT[6];
        temp_FT[0] = resp.FTData[0]; //输出Fx
        temp_FT[1] = resp.FTData[1]; //输出Fy
        temp_FT[2] = resp.FTData[2]; //输出Fz
        temp_FT[3] = resp.FTData[3]; //输出Tx
        temp_FT[4] = resp.FTData[4]; //输出Ty
        temp_FT[5] = resp.FTData[5]; //输出Tz
    for(int i=0;i<6;++i)
        cout<<temp_FT[i]<<" , ";
    cout<<endl;
}


void ATI_sensor::read()
{
    send(clisock, (const char*)request, 8, 0); //发送请求，接受数据
    recv(clisock, (char*)response, 36, 0);
    for (int i = 0; i < 6; i++)
            resp.FTData[i] = ntohl(*(int32*)&response[12 + i * 4])- resp.FTZero[i];
    int32 temp_FT[6];
        temp_FT[0] = resp.FTData[0]; //输出Fx
        temp_FT[1] = resp.FTData[1]; //输出Fy
        temp_FT[2] = resp.FTData[2]; //输出Fz
        temp_FT[3] = resp.FTData[3]; //输出Tx
        temp_FT[4] = resp.FTData[4]; //输出Ty
        temp_FT[5] = resp.FTData[5]; //输出Tz
    clock_gettime(CLOCK_REALTIME, &now);
    time_pass=now.tv_sec-begin_time.tv_sec;
    cout<<time_pass<<'s';
    of<<time_pass<<'s';
    for(int i=0;i<6;++i)
    {
        cout<<temp_FT[i]<<" , ";
        of<<temp_FT[i]<<" , ";
    }
    cout<<endl;
    of<<endl;
    // check overrun
    double overrun_time = ((now.tv_sec + double(now.tv_nsec)/NSEC_PER_SECOND) -  (before.tv_sec + double(before.tv_nsec)/NSEC_PER_SECOND))-period_usec;
    if (overrun_time > 0.0)
      {
        fprintf(stderr, "  overrun: %f", overrun_time);
      }
    before=now;
    ++counts;
    if (counts>=counts_totle)
    {
        uninit_time();
    }
}

void ATI_sensor::filter()
{
    send(clisock, (const char*)request, 8, 0); //发送请求，接受数据
    recv(clisock, (char*)response, 36, 0);
   
    //	resp.rdt_sequence = ntohl(*(uint32*)&response[0]);  //状态
    //	resp.ft_sequence = ntohl(*(uint32*)&response[4]);
    //	resp.status = ntohl(*(uint32*)&response[8]);

    for (int i = 0; i < 6; i++)
        resp.FTData[i] = ntohl(*(int32*)&response[12 + i * 4])- resp.FTZero[i];
    for (int i = 0; i < (SourceNum-1); ++i)
        respSource[i] = respSource[i + 1];
    respSource[SourceNum-1] = resp;
    RESPONSE resp_aver;
    for (int i = 0; i < 6; ++i)
    {
        int sum = 0;
        for (int j = 0; j < SourceNum; ++j)
            sum += respSource[j].FTData[i];
        resp_aver.FTData[i] = sum / SourceNum;
    }
    int32 temp_FT[6];
    temp_FT[0] = resp_aver.FTData[0]; //输出Fx
    temp_FT[1] = resp_aver.FTData[1]; //输出Fy
    temp_FT[2] = resp_aver.FTData[2]; //输出Fz
    temp_FT[3] = resp_aver.FTData[3]; //输出Tx
    temp_FT[4] = resp_aver.FTData[4]; //输出Ty
    temp_FT[5] = resp_aver.FTData[5]; //输出Tz
    for(int i=0;i<6;++i)
        cout<<temp_FT[i]<<" , ";
    cout<<endl;
}

void ATI_sensor::zero()
{
    send(clisock, (const char*)request, 8, 0); //发送请求，接受数据
    recv(clisock, (char*)response, 36, 0);
    for (int i = 0; i < 6; ++i)
        resp.FTZero[i] = ntohl(*(int32*)&response[12 + i * 4]);
}

void ATI_sensor::read_all()
{
    filter();
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
}

void ATI_sensor::create_thread()
{
    thrd = boost::thread(boost::bind(&read_all));
}

void ATI_sensor::destroy_thread()
{
    thrd.interrupt();
    thrd.join();
}
/*
int main(int argc, char *argv[])
{
       int b;
Loop:
       cout<<endl<<"Please choose which mini45 will do."<<endl<<"1:Zero,2:Read once,3:Read continually,4:Filter"<<endl;
       //cout<<"Please imput the number."<<endl;
       cin>>b;

       if(b==1)
       {
           send(clisock, (const char*)request, 8, 0); //发送请求，接受数据
           recv(clisock, (char*)response, 36, 0);
           for (int i = 0; i < 6; ++i)
               resp.FTZero[i] = ntohl(*(int32*)&response[12 + i * 4]);
           goto Loop;
       }
       else if(b==2)
       {
       read_once();
       goto Loop;
       }
       else if(b==3)
       {
           of.open("data.txt");
           if(!of)
               cout<<"File was created failed!"<<endl;
           signal(SIGALRM, read);
           set_timer();
           //while(1);
           of.close();
           goto Loop;
       }
       else if(b==4)
       {
           signal(SIGALRM, filter);
           set_timer();
           while(1);
       }
       else
       {
           cout<<"Please point 1,2,3or4."<<endl;
           goto Loop;
       }

       return 0;
}
*/