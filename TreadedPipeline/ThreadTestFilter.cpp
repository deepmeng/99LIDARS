#include <iostream>
#include <pcl/point_types.h>
#include <pcl/filters/passthrough.h>

#include <pthread.h>

#include <pcl/common/common_headers.h>
#include <pcl/features/normal_3d.h>
#include <pcl/io/pcd_io.h>
#include <pcl/visualization/pcl_visualizer.h>
#include <pcl/console/parse.h>
#include <pcl/console/time.h>

#include <pcl/sample_consensus/method_types.h>
#include <pcl/sample_consensus/model_types.h>
#include <pcl/segmentation/sac_segmentation.h>
#include <pcl/filters/extract_indices.h>

#include <pcl/surface/convex_hull.h>
#include <pcl/segmentation/extract_polygonal_prism_data.h>
#include <pcl/visualization/cloud_viewer.h>

#define NUM_THREADS 4

#define RANSAC_DISTANCE_THRESHOLD 0.25


/* This struct gets passed on to every
	worker thread created. */
struct thread_data{
	int thread_id;
	char *message;
	
	pcl::PointCloud<pcl::PointXYZ>::Ptr cloud;
};

/* Used to store the specific limits used by the passthrough filter */
struct region_limits{
		int xmin;
		int xmax;
		int ymin;
		int ymax;
};

/*Sets the region limits based on the thread id */
void specify_limits(int thread_id, region_limits *limits){
	switch(thread_id) {
		case 0 : limits->xmin = -100; limits->xmax = 0; limits->ymin = 0; limits->ymax = 100; break; 
		case 1 : limits->xmin = 0; limits->xmax = 100; limits->ymin = 0; limits->ymax = 100; break; 
		case 2 : limits->xmin = -100; limits->xmax = 0; limits->ymin = -100; limits->ymax = 0; break; 
		case 3 : limits->xmin = 0; limits->xmax = 100; limits->ymin = -100; limits->ymax = 0; break; 
	}

}

void *planar_segmentation(void *threadarg)
{
	pcl::console::TicToc tt;
	tt.tic();

	struct region_limits limits;
	struct thread_data *my_data;
    
    my_data = (struct thread_data *) threadarg;

    specify_limits(my_data->thread_id, &limits);

    
//	std::cout << "my_data->cloud: " << my_data->cloud->points.size () << " data points.\n" << std::endl;

    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_filt (new pcl::PointCloud<pcl::PointXYZ>);

	pcl::PassThrough<pcl::PointXYZ> passx;
	passx.setInputCloud (my_data->cloud);
	passx.setFilterFieldName ("x");
	passx.setFilterLimits (limits.xmin , limits.xmax);
	//pass.setFilterLimitsNegative (true);
    //pcl::PointCloud<pcl::PointXYZ>::Ptr cloud22 (new pcl::PointCloud<pcl::PointXYZ>);
	passx.filter (*cloud_filt);

	pcl::PassThrough<pcl::PointXYZ> passy;
	passy.setInputCloud (cloud_filt);
	passy.setFilterFieldName ("y");
	passy.setFilterLimits (limits.ymin , limits.ymax);
	//pass.setFilterLimitsNegative (true);
    //pcl::PointCloud<pcl::PointXYZ>::Ptr cloud22 (new pcl::PointCloud<pcl::PointXYZ>);
	passy.filter (*cloud_filt);


    std::cout << my_data->thread_id << ": " << cloud_filt->points.size () << " data points.\n" << std::endl;
	

	// Create the segmentation object for the planar model and set all the parameter
	pcl::SACSegmentation<pcl::PointXYZ> seg;
	pcl::PointIndices::Ptr inliers (new pcl::PointIndices);
	pcl::ModelCoefficients::Ptr coefficients (new pcl::ModelCoefficients);
	pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_plane (new pcl::PointCloud<pcl::PointXYZ> ());
	seg.setOptimizeCoefficients (true);
	seg.setModelType (pcl::SACMODEL_PLANE);
	seg.setMethodType (pcl::SAC_RANSAC);
	seg.setMaxIterations (100);
	seg.setDistanceThreshold (0.2);
	seg.setInputCloud (cloud_filt);

	// Segment the largest planar component from the remaining cloud
	seg.segment (*inliers, *coefficients);

	// Extract the planar inliers from the input cloud
	pcl::ExtractIndices<pcl::PointXYZ> extract;
	extract.setInputCloud (cloud_filt);
	extract.setIndices (inliers);
	extract.setNegative (false);

	// Get the points associated with the planar surface
	extract.filter (*cloud_filt);
	//std::cout << "PointCloud representing the planar component: " << my_data->cloud->points.size () << " data points." << std::endl;
    std::cout << "Thread: " << my_data->thread_id << "  time:" << tt.toc() << "ms\n";
    pthread_exit(NULL);
}


int main(int argc, char const *argv[])
{

	pthread_t threads[NUM_THREADS];
	struct thread_data td[NUM_THREADS];
	int rc;
	int i;
	pthread_attr_t attr;
    void *status;

	//Cloud object
	pcl::PointCloud<pcl::PointXYZ>::Ptr cloud (new pcl::PointCloud<pcl::PointXYZ>);
	//pcl::PointCloud<pcl::PointXYZ>::Ptr cloud0 (new pcl::PointCloud<pcl::PointXYZ>);
	//pcl::PointCloud<pcl::PointXYZ>::Ptr cloud1 (new pcl::PointCloud<pcl::PointXYZ>);
	//Timer object
    pcl::console::TicToc tt; 

    //Initialize and set thread joinable
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    //Read point cloud from file

    std::cerr << "Loding pcd-file to memory\n", tt.tic ();
    if (pcl::io::loadPCDFile<pcl::PointXYZ> ("data.pcd", *cloud) == -1) //* load the file
  	{
    	PCL_ERROR ("Couldn't read file data.pcd\n");
    	return (-1);
  	}
  	std::cerr << "Done: " << tt.toc () << "ms\n";



	// Create new threads
	for( i=0; i < NUM_THREADS; i++ ){
      //cout <<"main() : creating thread, " << i << endl;
      td[i].thread_id = i;
      //td[i].message = "This is message";
      td[i].cloud = cloud;
      //td[i].cloud2 = cloud2;

      rc = pthread_create(&threads[i], NULL, planar_segmentation, (void *)&td[i]);
      if (rc){
         cout << "Error:unable to create thread," << rc << endl;
         exit(-1);
      }
    }

    // free attribute and wait for the other threads
    pthread_attr_destroy(&attr);
    for( i=0; i < NUM_THREADS; i++ ){
       rc = pthread_join(threads[i], &status);
       if (rc){
          cout << "Error:unable to join," << rc << endl;
          exit(-1);
       }
       //cout << "Main: completed thread id :" << i ;
       //cout << "  exiting with status :" << status << endl;
    }


	return 0;
}