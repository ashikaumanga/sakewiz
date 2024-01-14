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

#include "appcontext.h"
#include <stdio.h>
#include <iostream>

using namespace std;

AppContext::AppContext()
{
    
}

std::map<std::string,std::string> AppContext::ctx;


std::string AppContext::getProperty(std::string k)
{
    return ctx[k];
}

void AppContext::init(std::string env)
{    
    if (env=="dev") {
        ctx["label_bucket"]="dev-com.sakewiz.images.labels";
        ctx["scan_bucket"]="dev-com.sakewiz.images.scans";
        ctx["region"]="us-east-1";
        
    } else if(env=="prd") {
        ctx["label_bucket"]="com.sakewiz.images.labels";
        ctx["scan_bucket"]="com.sakewiz.images.scans";
        ctx["region"]="ap-northeast-1";
    }
}



