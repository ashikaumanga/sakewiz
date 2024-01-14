/*
 * Copyright 2018 <copyright holder> <email>
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "wizscanendpoint.h"
#include <sstream>


WizScanEndpoint::WizScanEndpoint(Address addr)
: httpEndpoint(std::make_shared<Http::Endpoint>(addr))
{
    gpuMatcher = std::make_shared<GpuMatcher>();
}

void WizScanEndpoint::init(size_t thr) 
{

    auto opts = Http::Endpoint::options()
    .threads(thr)
    .flags(Tcp::Options::InstallSignalHandler).maxPayload(1024);
    //.max
    httpEndpoint->init(opts);
    setupRoutes();
    
}
void WizScanEndpoint::start()
{

    httpEndpoint->setHandler(router.handler());
    httpEndpoint->serve();
}
void WizScanEndpoint::shutdown()
{

    httpEndpoint->shutdown();
}
void WizScanEndpoint::setupRoutes()
{

    using namespace Rest;
    Routes::Get(router,"/wizscan3/init",
        Routes::bind(&WizScanEndpoint::doInitLabels, this));
    Routes::Get(router,"/wizscan3/scan/:scanid",
        Routes::bind(&WizScanEndpoint::doScan, this));
    Routes::Get(router,"/wizscan3/product/:productid/:labelid",
        Routes::bind(&WizScanEndpoint::doAddProduct, this));
    Routes::Get(router,"/wizscan3/gpuStatus",
        Routes::bind(&WizScanEndpoint::doGPUStatus, this));
    
}


void WizScanEndpoint::doInitLabels(const Rest::Request& request, Http::ResponseWriter response)
{
    g_lock.lock();
    auto hndl =std::async(std::launch::async, [&]{ gpuMatcher->init(); }); 
    g_lock.unlock();
    auto res = response.send(Http::Code::Ok, "{}");
    
}

std::string vectorToJson(std::vector<std::string> matches) {
    std::stringstream ss;
    ss<<"[";
    int c=0;
    for(auto const& prdId : matches) {
      if(c>0) ss << ",\n";
      ss << "\"" << prdId << "\"";
      c++;
    }
    ss<<"]";
    return ss.str();
    
}

std::string mapToJson(std::map<std::string,std::string> statusmap) {

    std::stringstream ss;
    ss<<"{";
    int c=0;
    for (auto const& x : statusmap) 
    {
      if(c>0) ss << ",\n";
      ss << "\"" << x.first << "\":\""<< x.second << "\"";
      c++;
    }
    ss<<"}";
    return ss.str();
}
void WizScanEndpoint::doAddProduct(const Rest::Request& request, Http::ResponseWriter response)
{
    string productid = request.param(":productid").as<std::string>();
    string labelid = request.param(":labelid").as<std::string>();
    int ret = gpuMatcher->addProduct(productid, labelid);
    if(ret==0)
    {
        response.send(Http::Code::Ok, "addProduct "+productid);
    }else
    {
        response.send(Http::Code::Internal_Server_Error, "addProduct "+productid);
    }
    
    
}
void WizScanEndpoint::doGPUStatus(const Rest::Request& request, Http::ResponseWriter response)
{
    response.headers()
                        .add<Header::Server>("pistache/0.1")
                        .add<Header::ContentType>(MIME(Application, Json));

    
    std::string ret = mapToJson(gpuMatcher->getStatus());
    response.send(Http::Code::Ok, ret);
}
void WizScanEndpoint::doScan(const Rest::Request& request, Http::ResponseWriter response)
{

    string scanid = request.param(":scanid").as<std::string>();
    if (scanid.empty()) {
        response.send(Http::Code::Bad_Request, "{}");
        return;
    }
    response.headers()
                        .add<Header::Server>("pistache/0.1")
                        .add<Header::ContentType>(MIME(Application, Json));
                        
    try {
      std::vector<std::string> m = gpuMatcher->scan(scanid);
      for(auto pid : m) {
       std::cout<<"  productId "<< pid << std::endl;
      }
      std::string ret = vectorToJson(m);
      response.send(Http::Code::Ok, ret);
    } catch (const std::exception &exc) {
      std::cout << exc.what();
      std::cout << "Error scan\n";
      response.send(Http::Code::Ok, "[]");
    }
    
    
}




