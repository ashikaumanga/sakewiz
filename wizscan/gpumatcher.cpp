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

#include "gpumatcher.h"


GpuMatcher::GpuMatcher() : options() ,config(), labelCount(0) ,  surf(200, 4,3,false,0.04f, false) 
{
    this->labelBucket=AppContext::getProperty("label_bucket");
    this->scanBucket=AppContext::getProperty("scan_bucket");
    this->duringInit=false;
    this->status = "idle";
    std::cout << "Label bucket " << this->labelBucket << std::endl;
    std::cout << "Scan bucket " << this->scanBucket << std::endl;
    populate_region(config);
    
    //
    Aws::InitAPI(options);
}

int GpuMatcher::addProduct(std::string& productId, std::string& labelid)
{

    
    {
        std::string labelKey = "/product-label/" + productId +"/"+ labelid+".jpeg";
        Aws::String aws_bucket(labelBucket.c_str(), labelBucket.size());
        Aws::String aws_key(labelKey.c_str(), labelKey.size());
        
    
        Aws::S3::S3Client s3_client(config);
        std::cout << "Product label " << productId << std::endl;


        Aws::S3::Model::GetObjectRequest objectReq;
        objectReq.WithBucket(aws_bucket);
        objectReq.SetKey(aws_key);
        auto get_object_outcome = s3_client.GetObject(objectReq);
        
        if (get_object_outcome.IsSuccess())
        {
            Aws::OFStream local_file;
            std::string path = std::tmpnam(nullptr);
            local_file.open(path, std::ios::out | std::ios::binary);
            local_file << get_object_outcome.GetResult().GetBody().rdbuf(); 
            local_file.close();
            //process this file
            Mat src = imread(path,IMREAD_GRAYSCALE);
            remove(path.c_str());
            
            GpuMat gpuSrc(src);
            GpuMat qkeypointsGPU;
            GpuMat qdescriptorsGPU;
    
            surf(gpuSrc, GpuMat(), qkeypointsGPU, qdescriptorsGPU);
            std::vector<GpuMat> descriptorList;
            descriptorList.push_back(qdescriptorsGPU);
            
            matcher->add(descriptorList);
         
            
            //lock
            _mutex.lock();
            this->idToProductMap[labelCount] = productId;
            labelCount++;
            _mutex.unlock();
            std::cout << "Product label added " << productId << std::endl;
            //unlock
            return 0;
            
                        
            
        } else
        {
            std::cout << "product label error: " <<
            get_object_outcome.GetError().GetExceptionName() << " " <<
            get_object_outcome.GetError().GetMessage() << std::endl;
            return -1;
        }
        
        
    }
    
    
}

void GpuMatcher::init()
{

    if(this->duringInit==true) return;
    this->duringInit=true;
    this->status = "loading";
    idToProductMap.clear();
    matcher->clear();
    loadFromS3();
    this->status = "ready";
    this->duringInit=false;
    
}

struct ImgMatch {
  int imgIdx;
  int count;
    
};

bool sortByCount(ImgMatch& m1,ImgMatch& m2) {
    return m1.count>m2.count;
}

std::vector<std::string> GpuMatcher::scan(std::string& scanId)
{
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    std::vector<std::string> ret;
    
        Aws::S3::S3Client s3_client(config);

        Aws::String aws_bucket(scanBucket.c_str(), scanBucket.size());
        Aws::S3::Model::GetObjectRequest objectReq;

        String prefix("/scan-img/");
        String postfix(".jpeg");
        String key=prefix+scanId+postfix;
        Aws::String aws_key(key.c_str(), key.size());
        objectReq.SetBucket(aws_bucket);
        objectReq.SetKey(aws_key);
        
        auto get_object_outcome = s3_client.GetObject(objectReq);
        if (get_object_outcome.IsSuccess())
        {
            Aws::OFStream local_file;
            std::string path = std::tmpnam(nullptr);
            local_file.open(path, std::ios::out | std::ios::binary);
            local_file << get_object_outcome.GetResult().GetBody().rdbuf(); 
            local_file.close();
            //process this file
            Mat src = imread(path,IMREAD_GRAYSCALE);
            remove(path.c_str());
            
            GpuMat gpuSrc(src);
            GpuMat qkeypointsGPU;
            GpuMat qdescriptorsGPU;
    
            surf(gpuSrc, GpuMat(), qkeypointsGPU, qdescriptorsGPU);
            std::vector<std::vector<DMatch>> matches;
            matcher->knnMatch(qdescriptorsGPU, matches,2);
            
            std::map<int,DMatch> qIdxToMinDistanceDMatch;
            for (auto &m : matches)
            {
         
                if(m[0].distance < 0.75*m[1].distance) {
                    int qIdx = m[0].queryIdx;
                    int tId = m[0].trainIdx;
                    int imgIdx = m[0].imgIdx;
            
                    //m[0].
                    if(qIdxToMinDistanceDMatch.count(qIdx)==0) {
                        qIdxToMinDistanceDMatch[qIdx]=m[0];
                    } else{
                        DMatch tmp = qIdxToMinDistanceDMatch[qIdx];
                        if (tmp.distance > m[0].distance) qIdxToMinDistanceDMatch[qIdx]=m[0];
                    }
                }
          
            }
            std::map<int,int> imgIdxToMatchCount;
            for(const auto& kv : qIdxToMinDistanceDMatch) {
                int imgIdx=kv.second.imgIdx;
                if(imgIdxToMatchCount.count(imgIdx)==0) {
                    imgIdxToMatchCount[imgIdx]=1; 
                } else {
                    int tmp=(imgIdxToMatchCount[imgIdx]);
                    tmp=tmp+1;
                    imgIdxToMatchCount[imgIdx]=tmp;
                }
         
            }
            //int medius=0;
            int sumCount =0;
            double avgCount=0,sumSq=0,varCount,stdDevCount=0;
            
             std::cout << "*************** scanId:" <<scanId << "**************************\r\n";
            
            std::vector<ImgMatch> imgMatches;
            for(const auto& kv:imgIdxToMatchCount) {
                ImgMatch tmp;
                tmp.imgIdx=kv.first;
                tmp.count=kv.second;
                sumCount += tmp.count;
                //sumSq+=(tmp.count * tmp.count);
                std::cout << "** "<< kv.first << " count "<<kv.second<<"\r\n";
                imgMatches.push_back(tmp);
        
            }
            int count=imgMatches.size();
            if (count == 0) return ret;
            avgCount=sumCount/count;
            //varCount = (count*sumSq - sumCount*sumCount)/(count*(count-1));
            //stdDevCount = sqrt(varCount);
            
            sort(imgMatches.begin(),imgMatches.end(), sortByCount);
            std::vector<ImgMatch> filteredMatches;
            for(auto& match : imgMatches) {
                std::cout << "count "<< match.count << " avg "<< avgCount
                << " stdDev " << stdDevCount << std::endl;
                if(match.count >= (avgCount)) {
                    filteredMatches.push_back(match);
                }else {
                
                    break;
                }
                
            }
            filteredMatches.resize(std::min((int)filteredMatches.size(),10));
            
            for (auto& m : filteredMatches) {
                ret.push_back(this->idToProductMap[m.imgIdx]);
            }
 
            
        }else {
            //TODO alert mail
            std::cout << "ListObjects error: " <<
            get_object_outcome.GetError().GetExceptionName() << " " <<
            get_object_outcome.GetError().GetMessage() << std::endl;
        }
        std::chrono::steady_clock::time_point end= std::chrono::steady_clock::now();
        std::cout << "********* Time difference = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() <<std::endl;
        
    
    
    return ret;
}
void GpuMatcher::loadFromS3()
{


    {

        std::cout << "****** Begin : loadFromS3()" << std::endl;
        Aws::S3::S3Client s3_client(config);
        Aws::S3::Model::ListObjectsRequest request;
        Aws::String aws_bucket(labelBucket.c_str(), labelBucket.size());
        request.WithBucket(aws_bucket).WithPrefix("product-label");

        bool isDone = false;
        Aws::S3::Model::ListObjectsOutcome outcome;
        Aws::S3::Model::GetObjectRequest objectReq;
        objectReq.WithBucket(aws_bucket);
        
        request.SetMaxKeys(1000);
        //int c=0;
        labelCount=0;
        std::vector<GpuMat> trainDescriptors;
        //process S3 bucket
        while(!isDone) {
            outcome=s3_client.ListObjects(request);
            if(!outcome.IsSuccess())
            { 
                std::cout << "ListObjects error: " <<
                outcome.GetError().GetExceptionName() << " " <<
                outcome.GetError().GetMessage() << std::endl;
                break;}
            //process
            Aws::Vector<Aws::S3::Model::Object> object_list = outcome.GetResult().GetContents();
            std::cout << "****** loadFromS3: process list " << std::endl;
            //process this set
            for (auto const &s3_object : object_list)
            {
            
                auto t = s3_object.GetKey();
                std::string key(t.c_str(), t.size());
                if (key.find("jpeg") == std::string::npos) continue;
                std::size_t found = key.find("-label");
                auto productId = key.substr(found+7,36);
               
                objectReq.SetKey(s3_object.GetKey());
                auto get_object_outcome = s3_client.GetObject(objectReq);
                if (get_object_outcome.IsSuccess())
                {
                    Aws::OFStream local_file;
                    std::string path = std::tmpnam(nullptr);
                    local_file.open(path, std::ios::out | std::ios::binary);
                    local_file << get_object_outcome.GetResult().GetBody().rdbuf(); 
                    local_file.close();
                    //process this file
                    Mat src = imread(path,IMREAD_GRAYSCALE);
                    remove(path.c_str()); 
                    
                    if(!src.data)
                    {
                        //TODO send alert mail
                        std::cout <<  "Could not open or find the image" << std::endl ;
                        continue;
                    }
                    
                    GpuMat gpuSrc;
                    int sizeRows=src.rows;
                    int sizeCols=src.cols;
                    if(src.rows<200 && src.cols>=200){
                        sizeRows=200;
                        sizeCols=src.cols*(200/(double)src.rows);
                    } else if(src.rows>=200 && src.cols<200) {
                        sizeCols=200;
                        sizeRows=src.rows*(200/(double)src.cols);
                    
                    } else if (src.rows<200 && src.cols<200) {
                        if(src.rows>src.cols){
                            sizeCols=200;
                            sizeRows=src.rows*(200/(double)src.cols);
                        } else {
                            sizeRows=200;
                            sizeCols=src.cols*(200/(double)src.rows);
                        }
                    }
                    if(sizeRows!=src.rows || sizeCols!=src.cols) {
                    //std::cout << "Prev rows:"<<src.rows<<" cols:"<<src.cols<<" New rows:"<<sizeRows<<" cols:"<<sizeCols<<"\r\n";
                        Mat dest;
                        cv::resize(src,dest,cv::Size(sizeCols,sizeRows));
                        gpuSrc.upload(dest);
                    }else {
                        gpuSrc.upload(src);
                    }
                    //
                    GpuMat keypointsGPU;
                    GpuMat descriptorsGPU;
                    //std::cout << ent2.path() << "\r\n";
                
                    try {
                        surf(gpuSrc, GpuMat(), keypointsGPU, descriptorsGPU);
                    } catch(...) {
                        std::cout << "Failed Row "<<path<<" orows:"<<src.rows<<" ocols:"<<src.cols <<" nrows:"<< sizeRows << " ncols: "<<sizeCols<<"\r\n";
                        continue;
                    }
                    trainDescriptors.push_back(descriptorsGPU);
                    
                    //end of process this file
                    this->idToProductMap[labelCount] = productId;
                    
                    labelCount++;
                    //if(labelCount%500==0) std::cout << "Count " << std::to_string(c) << std::endl;
                    //if(c>=50) break;
                    if (labelCount == 10) {
                                std::cout << "****** Loaded first 10 images" << std::endl;
                    }
                    if (labelCount % 500 ==0) {
                                std::cout << "****** Loaded another 500 images" << std::endl;
                    }
                                               
                    //std::cout << "file "<<path<< " " << key << std::endl;
                } else {
                
                    //TODO send alert mail here
                    std::cout << "Failed "<< key << std::endl;
                }
                
            }
            //end of process set
            std::cout << "****** loadFromS3: end process list ******" << std::endl;
            isDone=!outcome.GetResult().GetIsTruncated();
                 
            if(!isDone) {
                request.SetMarker(outcome.GetResult().GetContents().back().GetKey());
            }
            //if(c>=100) break;
        }
        //end of process S3 bucket
        matcher->add(trainDescriptors);
        
        //this->labelCount=c;
        std::cout << "labels " << std::to_string(labelCount) << std::endl;
        
    }
}

std::map<std::string,std::string> GpuMatcher::getStatus()
{
    std::map<std::string,std::string> ret;
    size_t free_mem, total_mem;
    cudaMemGetInfo(&free_mem, &total_mem);
    ret["status"]=this->status;
    ret["gpuTotalMemory"]=std::to_string(total_mem);
    ret["gpuFreeMemory"]=std::to_string(free_mem);
    ret["labelsLoaded"]=std::to_string(labelCount);
    return ret;
}

void GpuMatcher::shutdown()
{
    Aws::ShutdownAPI(options);
}




