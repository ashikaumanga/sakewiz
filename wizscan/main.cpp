#include <iostream>
#include <stdio.h>
#include "WizScan3App.h"
#include <vector>
#include <aws/core/Aws.h>
#include "pistache/endpoint.h"
#include "wizscanendpoint.h"

#include <signal.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/s3/model/ListObjectsV2Request.h>
#include <aws/s3/model/Object.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <fstream>
#include <map>
#include "appcontext.h"

using namespace Pistache;
using namespace std;


void my_handler(int s){
           printf("Caught signal %d\n",s);
           exit(1); 

}
int main(int argc, const char* argv[])
{

    
    
    int iport=8888;
    if (argc > 1)
    {
      std::string arg1(argv[1]);
      if(arg1=="dev" || arg1 == "prd") {
          AppContext::init(arg1);
          if(arg1=="dev") iport=18888;
      } else {
           std::cout << "prd or dev\n";
           return 1;
      }
    // do stuff with arg1
    } else {
      std::cout << "env not provided\n";
      return 1;
        
    }
    
    struct sigaction sigIntHandler;

   sigIntHandler.sa_handler = my_handler;
   sigemptyset(&sigIntHandler.sa_mask);
   sigIntHandler.sa_flags = 0;

   sigaction(SIGINT, &sigIntHandler, NULL);
    
    
    Port port(iport);
    
    Address addr(Ipv4::any(),port);
    WizScanEndpoint wizScan(addr);
    
    wizScan.init(10);
    wizScan.start();
    wizScan.shutdown();
    
    return 0;
}
