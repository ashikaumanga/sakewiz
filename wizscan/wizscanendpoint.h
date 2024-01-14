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

#ifndef WIZSCANENDPOINT_H
#define WIZSCANENDPOINT_H

#include "pistache/endpoint.h"
#include "pistache/http.h"
#include "pistache/router.h"

#include "gpumatcher.h"
#include <future>
#include <mutex>

using namespace std;
using namespace Pistache;
using namespace Pistache::Http;
/**
 * @todo write docs
 */
class WizScanEndpoint
{
private:
    std::mutex g_lock;
    std::shared_ptr<Http::Endpoint> httpEndpoint;
    std::shared_ptr<GpuMatcher> gpuMatcher;
    Rest::Router router;
    void setupRoutes();
    
    void doInitLabels(const Rest::Request& request,
                      Http::ResponseWriter response);
    void doScan(const Rest::Request& request,
                      Http::ResponseWriter response);
    void doAddProduct(const Rest::Request& request,
                      Http::ResponseWriter response);
    void doGPUStatus(const Rest::Request& request,
                      Http::ResponseWriter response);
    
public:
    /**
     * Default constructor
     */
    WizScanEndpoint(Address addr);
    void init(size_t thr =4);
    void start();
    void shutdown();

};

#endif // WIZSCANENDPOINT_H
