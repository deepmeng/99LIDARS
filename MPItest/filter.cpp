#include "filter.h"



Filters::Filters()
{
	// Empty constructor
}

int Filters::read_file(std::string infile, int nth_point)
{

	// load point cloud
	fstream input(infile.c_str(), ios::in | ios::binary);
	if(!input.good()){
		std::cerr << "Could not read file: " << infile << "\n";
		exit(EXIT_FAILURE);
	}
	input.seekg(0, ios::beg);

	float ignore;
	int i;
	for (i=0; input.good() && !input.eof(); i++) {
		pcl::PointXYZ point;
		input.read((char *) &point.x, 3*sizeof(float));
		input.read((char *) &ignore, sizeof(float));
		if(i%nth_point == 0)cloud->points.push_back(point);
	}
	input.close();

	float percent = ((float)(i/nth_point))/i;

	std::cout << "File have " << i << " points, " << "after filtering: " << (i/nth_point) << "  (" << percent << ")\n";

	
	return 0;		
}

int Filters::filter_and_slice(std::vector<std::vector<float> > *floats)
{

	  double zero = 0.0000000;
	  for (int iii = 0; iii < static_cast<int> (cloud->size()); ++iii){ 
    	if(cloud->points[iii].x > zero){
	          if(cloud->points[iii].y > zero){
	              if(cloud->points[iii].y > cloud->points[iii].x){
	              	  floats->at(0).push_back(cloud->points[iii].x);
	              	  floats->at(0).push_back(cloud->points[iii].y);
	              	  floats->at(0).push_back(cloud->points[iii].z);
	              }else{
	                  floats->at(1).push_back(cloud->points[iii].x);
	              	  floats->at(1).push_back(cloud->points[iii].y);
	              	  floats->at(1).push_back(cloud->points[iii].z);
	              }
	          }else{
	              if((abs(cloud->points[iii].y)) > cloud->points[iii].x){
	                  floats->at(2).push_back(cloud->points[iii].x);
	              	  floats->at(2).push_back(cloud->points[iii].y);
	              	  floats->at(2).push_back(cloud->points[iii].z);
	              }else{
	                  floats->at(3).push_back(cloud->points[iii].x);
	              	  floats->at(3).push_back(cloud->points[iii].y);
	              	  floats->at(3).push_back(cloud->points[iii].z);
	              }
	          }    
	      }else{
	          if(cloud->points[iii].y > zero){
	              if(cloud->points[iii].y > (abs(cloud->points[iii].x))){
	              	  floats->at(4).push_back(cloud->points[iii].x);
	              	  floats->at(4).push_back(cloud->points[iii].y);
	              	  floats->at(4).push_back(cloud->points[iii].z);
	               }else{
	                  floats->at(5).push_back(cloud->points[iii].x);
	              	  floats->at(5).push_back(cloud->points[iii].y);
	              	  floats->at(5).push_back(cloud->points[iii].z);
	               }
	           }else{
	               if(cloud->points[iii].y > cloud->points[iii].x){
	                  floats->at(6).push_back(cloud->points[iii].x);
	              	  floats->at(6).push_back(cloud->points[iii].y);
	              	  floats->at(6).push_back(cloud->points[iii].z);
	                }else{
	                  floats->at(7).push_back(cloud->points[iii].x);
	              	  floats->at(7).push_back(cloud->points[iii].y);
	              	  floats->at(7).push_back(cloud->points[iii].z);
	                }
	          }
	      }
   
  }
	
	return 0;		
}
