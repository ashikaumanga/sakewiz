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

#ifndef GPUMATCHER_H
#define GPUMATCHER_H
#include <stdio.h>
#include <string>
#include <map>
#include <aws/core/Aws.h>
#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include <cuda_runtime.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/s3/model/ListObjectsV2Request.h>
#include <aws/s3/model/Object.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <cuda_runtime.h>

#include "appcontext.h"
#include <chrono>

using namespace cv;
using namespace cv::cuda;
/**
 * @todo write docs
 */
class GpuMatcher
{
private:
    Aws::SDKOptions options;
    Aws::Client::ClientConfiguration config;
    //Aws::S3::S3Client s3_client;
    std::string labelBucket;
    std::string scanBucket;
    std::string status;
    std::atomic<int> labelCount;
    int productCount=0;
    std::map<int,std::string> idToProductMap;
    std::mutex _mutex;
    Ptr<cv::cuda::DescriptorMatcher> matcher =  cv::cuda::DescriptorMatcher::createBFMatcher(cv::NORM_L1);
    cv::cuda::SURF_CUDA surf;//(400, 4,2,false,0.01f, false);
    void loadFromS3();
    
    static Aws::Client::ClientConfiguration generateConfig() {
        std::cout<<"gpumatcher inline\n";
        Aws::Client::ClientConfiguration config;
        std::string region=AppContext::getProperty("region");
        Aws::String aws_region(region.c_str(), region.size());
        config.region=aws_region;
        std::cout<<"end gpumatcher inline\n";
        return config;
    } 
    
    static const Aws::Client::ClientConfiguration& populate_region(Aws::Client::ClientConfiguration& config){
       std::string region=AppContext::getProperty("region");
       Aws::String aws_region(region.c_str(), region.size());
       config.region=aws_region;
    }
public:
    /**
     * Default constructor
     */
    GpuMatcher();
    bool duringInit;
    void init();
    void shutdown();
    std::vector<std::string> scan(std::string& scanId);
    int addProduct(std::string& productId, std::string& labelid);
    std::map<std::string,std::string> getStatus();

};

#endif // GPUMATCHER_H
