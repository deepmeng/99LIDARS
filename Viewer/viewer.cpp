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

int main (int argc, char** argv)
{


	 // Timer object
  pcl::console::TicToc tt;


  tt.tic();

 // Read in the cloud data
   std::string infile = "../../PCDdataFiles/002.bin";

	// load point cloud
	fstream input(infile.c_str(), ios::in | ios::binary);
	if(!input.good()){
		cerr << "Could not read file: " << infile << endl;
		exit(EXIT_FAILURE);
	}
	input.seekg(0, ios::beg);

	//pcl::PointCloud<PointXYZ>::Ptr cloud (new pcl::PointCloud<PointXYZ>);
	pcl::PointCloud<pcl::PointXYZ>::Ptr cloud (new pcl::PointCloud<pcl::PointXYZ>);

	float ignore;
	int i;
	for (i=0; input.good() && !input.eof(); i++) {
		pcl::PointXYZ point;
		input.read((char *) &point.x, 3*sizeof(float));
		input.read((char *) &ignore, sizeof(float));
		cloud->push_back(point);
	}
	input.close();

	cout << "Read KTTI point cloud with " << cloud->points.size() << " points in " << tt.toc() << " ms." << endl;


  std::fstream myfile("clusters.txt", std::ios_base::in);

  std::vector<float> v;

    float a;
    while (myfile >> a)
    {
        v.push_back(a);
    }
   // ----------------------------------------------------------------------------------------------------------
   // -----Open 3D viewer and add point cloud-----
   //pcl::visualization::PointCloudColorHandlerCustom<pcl::PointXYZRGB> grey(cloud,204, 204, 179);
   //pcl::visualization::PointCloudColorHandlerCustom<pcl::PointXYZRGB> red(cloud, 255, 0, 0);
    boost::shared_ptr<pcl::visualization::PCLVisualizer> viewer (new pcl::visualization::PCLVisualizer ("3D Viewer"));
   viewer->setBackgroundColor (0, 0, 0);
   viewer->addPointCloud<pcl::PointXYZ> (cloud, "source");
   viewer->setPointCloudRenderingProperties (pcl::visualization::PCL_VISUALIZER_COLOR, 0.2f, 0.2f, 0.2f, "source");
   //viewer->addPointCloud<pcl::PointXYZ> (cloud_main, "main");
   //viewer->setPointCloudRenderingProperties (pcl::visualization::PCL_VISUALIZER_COLOR, 1.0f, 0.0f, 0.0f, "main");  
   //viewer->setPointCloudRenderingProperties (pcl::visualization::PCL_VISUALIZER_POINT_SIZE, 1, "Cloud with boxes");
   viewer->addCoordinateSystem (1.0);
   viewer->initCameraParameters ();

   std::vector<pcl::PointXYZ> vpoints;


   std::stringstream ss;
	   for(int h = 0 ; h < v.size(); h++)
	   {
  		    	if(h%6 == 0){
  		    		ss << "id" << h << "test";
    				std::string str = ss.str();
  		    		viewer->addCube(v.at(h), v.at(h+3), v.at(h+1), v.at(h+4), v.at(h+2), v.at(h+5), 1.0,0.0,0.0, str ,0);
  		    	}
 	   }	
	  //------------------------------------------------------------------------------------------------------------

	  int z = -1.5;
	  viewer->addLine<pcl::PointXYZ> (pcl::PointXYZ(0,0,z),pcl::PointXYZ(35,0,z), "aline");
	  viewer->addLine<pcl::PointXYZ> (pcl::PointXYZ(0,0,z),pcl::PointXYZ(0,30,z), "bline");
	  viewer->addLine<pcl::PointXYZ> (pcl::PointXYZ(0,0,z),pcl::PointXYZ(-35,0,z), "cline");
	  viewer->addLine<pcl::PointXYZ> (pcl::PointXYZ(0,0,z),pcl::PointXYZ(0,-30,z), "dline");
	  viewer->addLine<pcl::PointXYZ> (pcl::PointXYZ(0,0,z),pcl::PointXYZ(20,20,z), "eline");
	  viewer->addLine<pcl::PointXYZ> (pcl::PointXYZ(0,0,z),pcl::PointXYZ(-20,20,z), "fline");
	  viewer->addLine<pcl::PointXYZ> (pcl::PointXYZ(0,0,z),pcl::PointXYZ(20,-20,z), "gline");
	  viewer->addLine<pcl::PointXYZ> (pcl::PointXYZ(0,0,z),pcl::PointXYZ(-20,-20,z), "hline");


	  while (!viewer->wasStopped ())
	  {
	    viewer->spinOnce (100);
	    boost::this_thread::sleep (boost::posix_time::microseconds (100000));
	  }
  
  return (0);
}