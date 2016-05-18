#include <pcl/ModelCoefficients.h>
#include <pcl/point_types.h>
#include <pcl/io/pcd_io.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/features/normal_3d.h>
#include <pcl/kdtree/kdtree.h>
#include <pcl/sample_consensus/method_types.h>
#include <pcl/sample_consensus/model_types.h>
#include <pcl/segmentation/sac_segmentation.h>
#include <pcl/segmentation/extract_clusters.h>
#include <pcl/console/time.h>
#include <pcl/filters/passthrough.h>
#include <string>
#include <pcl/common/common.h>

#include <pcl/visualization/cloud_viewer.h>
#include <pcl/features/moment_of_inertia_estimation.h>
#include <vector>
#include <pcl/console/parse.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm>

//************************************
// 			DBSCAN includes
#include "dbscan/dbscan.h"
#include "dbscan/utils.h"
#include "dbscan/kdtree2.hpp"
//************************************

#define OFFSET 0.02

struct object{
		pcl::PointXYZ minPt;
		pcl::PointXYZ maxPt;
		bool remove;
		bool merged;
};

bool DoObjectsIntersect(object a, object b) {
  	if(a.minPt.x > (OFFSET+b.maxPt.x)) return false;
  	if((a.maxPt.x+OFFSET) < b.minPt.x) return false;
  	if(a.minPt.y > (OFFSET+b.maxPt.y)) return false;
  	if((a.maxPt.y+OFFSET) < b.minPt.y) return false;
  	return true;
}

object MergeObjects(object a, object b){
	object c;
	float LeftCornerX = std::min(a.minPt.x, b.minPt.x);
	float LeftCornerY = std::min(a.minPt.y, b.minPt.y);
	float LeftCornerZ = std::min(a.minPt.z, b.minPt.z);

	float RightCornerX = std::max(a.maxPt.x, b.maxPt.x);
	float RightCornerY = std::max(a.maxPt.y, b.maxPt.y);
	float RightCornerZ = std::max(a.maxPt.z, b.maxPt.z);


	c.minPt = pcl::PointXYZ(LeftCornerX, LeftCornerY, LeftCornerZ);
	c.maxPt = pcl::PointXYZ(RightCornerX, RightCornerY, RightCornerZ);
	c.remove = false;
	c.merged = true;


	return c;
}

int main (int argc, char** argv)
{

  bool visualization = false;
  bool lines = false;
  bool dbscan = false;
  bool read_binary = true;
  int nth_point = 5; // five is default
  double eps = 0.5; // epsilon for clustering default 0.6 for the
  int minCl = 50;

  std::string infile = "../../Dataframes_txt/";
  //std::string infile = "../../BinAndTxt/";
  std::string file = "0000000021.bin";

  // --------------------------------------
  // -----Parse Command Line Arguments-----
  // --------------------------------------

  for (int i = 1; i < argc; i++) { /* We will iterate over argv[] to get the parameters stored inside.
                                          * Note that we're starting on 1 because we don't need to know the 
                                          * path of the program, which is stored in argv[0] */
            if (i != argc){ // Check that we haven't finished parsing already
                if(std::strcmp(argv[i], "-v") == 0) {
             		visualization = true;
                } else if(std::strcmp(argv[i], "-l") == 0) {
                    lines = true;
                } else if(std::strcmp(argv[i], "-d") == 0) {
                    dbscan = true;
                } else if(std::strcmp(argv[i], "-n") == 0){
 					sscanf(argv[i+1], "%i", &nth_point);
				} else if(std::strcmp(argv[i], "-e") == 0){
					eps = atof(argv[i+1]);
				} else if(std::strcmp(argv[i], "-m") == 0){
					minCl = atoi(argv[i+1]);
				} else if(std::strcmp(argv[i], "-i") == 0){
					file.assign(argv[i+1]);
				} else if(std::strcmp(argv[i], "-t") == 0){
					read_binary = false;
				}                                    
                           
            }
            //std::cout << argv[i] << " ";
        }

  //cout << endl << "Arg, n: " << nth_point << " eps: " << eps << " minCl: " << minCl << endl;

  pcl::console::TicToc tt;

  infile.append(file);
  // Read in the cloud data
  pcl::PCDReader reader;
  pcl::PointCloud<pcl::PointXYZ>::Ptr cloud (new pcl::PointCloud<pcl::PointXYZ>);
  pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_f (new pcl::PointCloud<pcl::PointXYZ>);
  pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_main (new pcl::PointCloud<pcl::PointXYZ>);

  std::vector<pcl::PointXYZ> points;


	if(read_binary){ // Binary
		// load point cloud
		fstream input(infile.c_str(), ios::in | ios::binary);
		if(!input.good()){
			cerr << "Could not read file: " << infile << endl;
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

		//cout << "File have " << i << " points, " << "after filtering: " << (i/nth_point) << "  (" << percent << ") "<< endl;
	}else{
		//  READ TXT INSTEAD OF BIN =======================================================================================
		FILE* f = fopen(infile.c_str(), "r");
		if (NULL == f) {
		   cerr << "Could not read file: " << infile << endl;
		   return 0;
		}
		
		int i = 0;
		float intensity;
		pcl::PointXYZ p;
		while(fscanf(f,"%f %f %f %f\n", &p.x, &p.y, &p.z, &intensity) == 4) {
			if(i%nth_point == 0) cloud->points.push_back(p);
			i++;
		}
		//cout << "file have " << i << " points" << endl;
		fclose(f);
	}       

	tt.tic();   

  pcl::PointCloud<pcl::PointXYZ>::Ptr cloud0 (new pcl::PointCloud<pcl::PointXYZ>);
  pcl::PointCloud<pcl::PointXYZ>::Ptr cloud1 (new pcl::PointCloud<pcl::PointXYZ>);
  pcl::PointCloud<pcl::PointXYZ>::Ptr cloud2 (new pcl::PointCloud<pcl::PointXYZ>);
  pcl::PointCloud<pcl::PointXYZ>::Ptr cloud3 (new pcl::PointCloud<pcl::PointXYZ>);
  pcl::PointCloud<pcl::PointXYZ>::Ptr cloud4 (new pcl::PointCloud<pcl::PointXYZ>);
  pcl::PointCloud<pcl::PointXYZ>::Ptr cloud5 (new pcl::PointCloud<pcl::PointXYZ>);
  pcl::PointCloud<pcl::PointXYZ>::Ptr cloud6 (new pcl::PointCloud<pcl::PointXYZ>);
  pcl::PointCloud<pcl::PointXYZ>::Ptr cloud7 (new pcl::PointCloud<pcl::PointXYZ>);

  std::vector<pcl::PointCloud<pcl::PointXYZ>::Ptr > v;
  v.push_back(cloud0);
  v.push_back(cloud1);
  v.push_back(cloud2);
  v.push_back(cloud3);
  v.push_back(cloud4);
  v.push_back(cloud5);
  v.push_back(cloud6);
  v.push_back(cloud7);

  // Devide the dataset and keep every n:th point (setting it to 1 will include all points)
  //int nth_point = 3;
  double zero = 0.0000000;
  for (int iii = 0; iii < static_cast<int> (cloud->size ()); ++iii){ 
   // if((iii%nth_point) == 0){
    	if(cloud->points[iii].x > zero){
	          if(cloud->points[iii].y > zero){
	              if(cloud->points[iii].y > cloud->points[iii].x){
	                  cloud0->points.push_back (pcl::PointXYZ (cloud->points[iii].x,cloud->points[iii].y,cloud->points[iii].z));
	              }else{
	                  cloud1->points.push_back (pcl::PointXYZ (cloud->points[iii].x,cloud->points[iii].y,cloud->points[iii].z));
	              }
	          }else{
	              if((abs(cloud->points[iii].y)) > cloud->points[iii].x){
	                  cloud3->points.push_back (pcl::PointXYZ (cloud->points[iii].x,cloud->points[iii].y,cloud->points[iii].z));
	              }else{
	                  cloud2->points.push_back (pcl::PointXYZ (cloud->points[iii].x,cloud->points[iii].y,cloud->points[iii].z));
	              }
	          }    
	      }else{
	          if(cloud->points[iii].y > zero){
	              if(cloud->points[iii].y > (abs(cloud->points[iii].x))){
	                      cloud7->points.push_back (pcl::PointXYZ (cloud->points[iii].x,cloud->points[iii].y,cloud->points[iii].z));
	                  }else{
	                      cloud6->points.push_back (pcl::PointXYZ (cloud->points[iii].x,cloud->points[iii].y,cloud->points[iii].z));
	                  }
	              }else{
	                  if(cloud->points[iii].y > cloud->points[iii].x){
	                      cloud5->points.push_back (pcl::PointXYZ (cloud->points[iii].x,cloud->points[iii].y,cloud->points[iii].z));
	                  }else{
	                      cloud4->points.push_back (pcl::PointXYZ (cloud->points[iii].x,cloud->points[iii].y,cloud->points[iii].z));
	                  }
	          }
	      }
  }


  //std::vector<std::vector<object> > objectsVector(8, vector<object>(0));
  std::vector<object> objects;  


  int clusters = 0;
  int ii = 0;
  std::vector<pcl::PointXYZ> cluster_vector;

  for(int ii = 0 ; ii < v.size(); ii++){

	  Eigen::Vector3f axis = Eigen::Vector3f(0.0,0.0,1.0);

	  pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_filtered (new pcl::PointCloud<pcl::PointXYZ>);
	  pcl::SACSegmentation<pcl::PointXYZ> seg;
	  pcl::PointIndices::Ptr inliers (new pcl::PointIndices);
	  pcl::ModelCoefficients::Ptr coefficients (new pcl::ModelCoefficients);
	  pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_plane (new pcl::PointCloud<pcl::PointXYZ> ());
	  pcl::PCDWriter writer;
	  seg.setEpsAngle( 15.0f * (M_PI/180.0f) ); // Perfect value! 
	  seg.setAxis(axis);
	  seg.setOptimizeCoefficients (true);
	  seg.setModelType (pcl::SACMODEL_PERPENDICULAR_PLANE);
	  seg.setMethodType (pcl::SAC_RANSAC);
	  seg.setMaxIterations (100);
	  seg.setDistanceThreshold (0.25); // 0.3
	  seg.setInputCloud (v.at(ii)); 					//change input cloud later 
	  seg.segment (*inliers, *coefficients);
	  // Extract the planar inliers from the input cloud
	  pcl::ExtractIndices<pcl::PointXYZ> extract;
	  extract.setInputCloud (v.at(ii));
	  extract.setIndices (inliers);
	  extract.setNegative (false);
	  // Get the points associated with the planar surface
	  extract.filter (*cloud_plane);
	  //std::cout << "PointCloud representing the planar component: " << cloud_plane->points.size () << " data points." << std::endl;
	  // Remove the planar inliers, extract the rest
	  extract.setNegative (true);
	  extract.filter (*cloud_f);
	  *cloud_filtered = *cloud_f;
	  
	  if(!dbscan){
		  // Creating the KdTree object for the search method of the extraction
		  pcl::search::KdTree<pcl::PointXYZ>::Ptr tree (new pcl::search::KdTree<pcl::PointXYZ>);
		  tree->setInputCloud (cloud_filtered);
		  std::vector<pcl::PointIndices> cluster_indices;
		  pcl::EuclideanClusterExtraction<pcl::PointXYZ> ec;
		  ec.setClusterTolerance (eps); // 0.02 = 2cm
		  ec.setMinClusterSize (minCl);
		  //ec.setMaxClusterSize (); // with voxel it should be aroud 5000
		  ec.setSearchMethod (tree);
		  ec.setInputCloud (cloud_filtered);
		  ec.extract (cluster_indices);
		  

		  int j = 0;
		  for (std::vector<pcl::PointIndices>::const_iterator it = cluster_indices.begin (); it != cluster_indices.end (); ++it)
		  {
		    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_cluster (new pcl::PointCloud<pcl::PointXYZ>);
		    clusters++;
		    for (std::vector<int>::const_iterator pit = it->indices.begin (); pit != it->indices.end (); ++pit)
		      cloud_cluster->points.push_back (cloud_filtered->points[*pit]); //*
		      cloud_cluster->width = cloud_cluster->points.size ();
		      cloud_cluster->height = 1;
		      cloud_cluster->is_dense = true;
		    
		    
		    object obj;
		    obj.remove = false;
		    obj.merged = false;
	 		pcl::getMinMax3D (*cloud_cluster, obj.minPt, obj.maxPt);
	 		objects.push_back(obj);
	 	
	 		j++;
		    cloud_cluster->points.clear();
		  }

		  cloud_filtered->points.clear();
		  cloud_f->points.clear();
		}else{ // DBSCAN CODE

			tt.tic();
			int num_threads = 4;
			//int minPts = 30; // minimal amout of points in order to be considered a cluster
			//double eps = 0.6; // distance between points


			omp_set_num_threads(num_threads); // Use 4 threads for clustering on the odroid

			NWUClustering::ClusteringAlgo dbs;
			dbs.set_dbscan_params(eps, minCl);

			double start = omp_get_wtime();
			//cout << "DBSCAN reading points.."<< endl;
			dbs.read_cloud(cloud_filtered);	

			//cout << "Reading input data file took " << omp_get_wtime() - start << " seconds." << endl;

			// build kdtree for the points
			start = omp_get_wtime();
			dbs.build_kdtree();
			//cout << "Build kdtree took " float c_buff [200];<< omp_get_wtime() - start << " seconds." << endl;

			start = omp_get_wtime();
			//run_dbscan_algo(dbs);
			run_dbscan_algo_uf(dbs);
			//cout << "DBSCAN (total) took " << omp_get_wtime() - start << " seconds." << endl;


			// Calculate boxes from all the clusters found
			dbs.writeClusters_uf(&cluster_vector);

			int num_cl = 0;
			for (int i = 0; i < cluster_vector.size(); ++i)
			{
				if(i%2 == 0)
				{
					object obj;
					obj.minPt = cluster_vector.at(i);
					obj.maxPt = cluster_vector.at(i+1);
					obj.remove = false;
					objects.push_back(obj);
				}
			}
			
			cluster_vector.clear();
		  	//cout << (buffer_size/6) << " clusters." << endl;
		}
  } //End sector for


  	/* Merging of boxes*/

  	std::vector<object> objv;

  	//cout << "before merging: " << objects.size() << endl;

  	for (int i = 0; i < objects.size(); ++i)
  	{	
		object a = objects.at(i);
		
		for (int ii = i; ii < objects.size(); ++ii)
		{
			
			object b = objects.at(ii);

			if(DoObjectsIntersect(a,b) && i != ii){

				object merge = MergeObjects(a,b);
				objv.push_back(a);
				objv.push_back(b);
				objects.at(ii) = merge;
				objects.at(i).remove = true;
			}
  		}
  	}


  if(visualization){
	  // ----------------------------------------------------------------------------------------------------------
	  // -----Open 3D viewer and add point cloud-----
	  //pcl::visualization::PointCloudColorHandlerCustom<pcl::PointXYZRGB> grey(cloud,204, 204, 179);
	  //pcl::visualization::PointCloudColorHandlerCustom<pcl::PointXYZRGB> red(cloud, 255, 0, 0);
  	  boost::shared_ptr<pcl::visualization::PCLVisualizer> viewer (new pcl::visualization::PCLVisualizer ("3D Viewer"));
	  viewer->setBackgroundColor (0, 0, 0);
	  viewer->addPointCloud<pcl::PointXYZ> (cloud, "source");
	  float intz = 0.6f;
	  viewer->setPointCloudRenderingProperties (pcl::visualization::PCL_VISUALIZER_COLOR, intz, intz, intz, "source");
	  //viewer->addPointCloud<pcl::PointXYZ> (cloud_main, "main");
	  //viewer->setPointCloudRenderingProperties (pcl::visualization::PCL_VISUALIZER_COLOR, 1.0f, 0.0f, 0.0f, "main");  
	  //viewer->setPointCloudRenderingProperties (pcl::visualization::PCL_VISUALIZER_POINT_SIZE, 1, "Cloud with boxes");
	  viewer->addCoordinateSystem (1.0);
	  viewer->initCameraParameters ();

	  double  pos_x = 0;
	  double  pos_y = -10; // -13
	  double  pos_z = 19; // 10
	  double  up_x = 0;
	  double  up_y = 1;
	  double  up_z = 1;

	  viewer->setCameraPosition(pos_x,pos_y,pos_z,up_x,up_y,up_z, 0); 		

	  std::stringstream ss;
	  int counts = 0;
	  
		/* ====== PRINTING ALL THE BOXES ========================== */

  		for (int i = 0; i < objv.size(); ++i)
			{
	  		object obj = objv.at(i);
  			ss << "id" << i << "t";
			std::string str = ss.str();
		    	
		    	viewer->addCube(obj.minPt.x, obj.maxPt.x, obj.minPt.y, obj.maxPt.y, obj.minPt.z, obj.maxPt.z, 0.0,0.0,0.7, str ,0);
	    		ss.str("");
  		}

	  	for (int i = 0; i < objects.size(); ++i)
			{
	  		object obj = objects.at(i);
	  			if(!obj.remove)
	  			{
		  			ss << "id" << counts << "test";
	    			std::string str = ss.str();
	  		    	pcl::PointXYZ middle;
	  		    	if(obj.merged)
  		    			viewer->addCube(obj.minPt.x, obj.maxPt.x, obj.minPt.y, obj.maxPt.y, obj.minPt.z, obj.maxPt.z, 1.0,1.0,0.0, str ,0);
  		    		else
  		    			viewer->addCube(obj.minPt.x, obj.maxPt.x, obj.minPt.y, obj.maxPt.y, obj.minPt.z, obj.maxPt.z, 1.0,0.0,0.0, str ,0);
  		    		ss.str("");
    				ss << counts;
    				middle = pcl::PointXYZ(((obj.minPt.x+obj.maxPt.x)/2),((obj.minPt.y+obj.maxPt.y)/2),((obj.minPt.z+obj.maxPt.z)/2));
    				viewer->addText3D(ss.str(),middle, 0.5,1.0,1.0,1.0,ss.str(),0);
    				counts++;
  		    	}
	  	}
	  	cout << counts << endl;

	  if(lines){
	  	int z = -1.9;
	  	viewer->addLine<pcl::PointXYZ> (pcl::PointXYZ(0,0,z),pcl::PointXYZ(55,0,z),0.0f,8.0f,0.0f, "aline");
	  	viewer->addLine<pcl::PointXYZ> (pcl::PointXYZ(0,0,z),pcl::PointXYZ(0,55,z),0.0f,8.0f,0.0f, "bline");
	  	viewer->addLine<pcl::PointXYZ> (pcl::PointXYZ(0,0,z),pcl::PointXYZ(-55,0,z),0.0f,8.0f,0.0f, "cline");
	  	viewer->addLine<pcl::PointXYZ> (pcl::PointXYZ(0,0,z),pcl::PointXYZ(0,-55,z),0.0f,8.0f,0.0f, "dline");
	  	viewer->addLine<pcl::PointXYZ> (pcl::PointXYZ(0,0,z),pcl::PointXYZ(55,55,z),0.0f,8.0f,0.0f, "eline");
	  	viewer->addLine<pcl::PointXYZ> (pcl::PointXYZ(0,0,z),pcl::PointXYZ(-55,55,z),0.0f,8.0f,0.0f, "fline");
	  	viewer->addLine<pcl::PointXYZ> (pcl::PointXYZ(0,0,z),pcl::PointXYZ(55,-55,z),0.0f,8.0f,0.0f, "gline");
	  	viewer->addLine<pcl::PointXYZ> (pcl::PointXYZ(0,0,z),pcl::PointXYZ(-55,-55,z),0.0f,8.0f,0.0f, "hline");
	  }
	   //std::cout << "Found a total of: " << clusters << " clusters." << endl;

	  while (!viewer->wasStopped ())
	  {
	    viewer->spinOnce (100);
	    boost::this_thread::sleep (boost::posix_time::microseconds (100000));
	  }
  	}else{
	  	
	  	//count all clusters
	  	//int counts = 0;
	  	//for (int i = 0; i < objects.size(); ++i)
  		//	if(!objects.at(i).remove) counts++;

		cout << tt.toc();
		//cout << counts;
	  
  	}

  return (0);
}