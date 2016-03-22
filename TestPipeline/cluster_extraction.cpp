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
<<<<<<< HEAD
#include <string>
=======
#include "include/util.h"
#include <string>
#include <sstream>
>>>>>>> 7675a631730c3f69fae22f9d2df7b4060bc47f22

#include <pcl/visualization/cloud_viewer.h>
#include <pcl/features/moment_of_inertia_estimation.h>
#include <vector>

//Value to string converter

int 
main (int argc, char** argv)
{

  // Read in the cloud data
  pcl::PCDReader reader;
  pcl::PointCloud<pcl::PointXYZI>::Ptr cloud (new pcl::PointCloud<pcl::PointXYZI>), cloud_f (new pcl::PointCloud<pcl::PointXYZI>);
  pcl::PointCloud<pcl::PointXYZI>::Ptr cloud_filtered (new pcl::PointCloud<pcl::PointXYZI>);
  // Timer object
  pcl::console::TicToc tt;
  pcl::console::TicToc pT;

  // Begin looping through cloud data.
  for(int i=0; i < 5; i++){
    pT.tic();

    std::string str = std::string("data0") + to_string(i) + ".pcd";
    reader.read(str, *cloud);
    std::cout << str << " :PointCloud before filtering has: " << cloud->points.size () << " data points." << std::endl;
    
    // Create the pass through filtering object
    // COMMENT BELOW SEGMENT TO REMOVE PASSTHROUGH FILTERING
    //std::cout << str << " :Running passthrough downsampling\n", tt.tic();
    //pcl::PassThrough<pcl::PointXYZI> pass;
    //pass.setInputCloud (cloud);
    //pass.setFilterFieldName ("x");
    //pass.setFilterLimits (-100, 0);
    //pass.setFilterLimitsNegative (true);
    //pass.filter (*cloud_filtered);
    //pass.setInputCloud(cloud_filtered);
    //pass.setFilterFieldName("y");
    //pass.setFilterLimits(-100, 0);
    //pass.filter(*cloud_filtered);
    //std::cout << str << " >> Done: " << tt.toc () << " ms\n";
    //COMMENT ABOVE SEGMENT TO REMOVE PASSTHROUGH FILTERING

    std::cout << str << " :Starting VoxelGrid downsampling\n",tt.tic ();
    // Create the filtering object: downsample the dataset using a leaf size of 7cm
    pcl::VoxelGrid<pcl::PointXYZI> vg;
    //pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_filtered (new pcl::PointCloud<pcl::PointXYZ>);
    vg.setInputCloud (cloud);
    vg.setLeafSize (0.07f, 0.07f, 0.07f);
    vg.filter (*cloud_filtered);
    std::cout << str << " >> Done: " << tt.toc () << " ms\n";
    std::cout << str << " :PointCloud after filtering has: " << cloud_filtered->points.size ()  << " data points." << std::endl; //*

    // Create the segmentation object for the planar model and set all the parameters
    std::cout << str << " :Starting Planar Segmentation",tt.tic ();
    pcl::SACSegmentation<pcl::PointXYZI> seg;
    pcl::PointIndices::Ptr inliers (new pcl::PointIndices);
    pcl::ModelCoefficients::Ptr coefficients (new pcl::ModelCoefficients);
    pcl::PointCloud<pcl::PointXYZI>::Ptr cloud_plane (new pcl::PointCloud<pcl::PointXYZI> ());
    pcl::PCDWriter writer;
    seg.setOptimizeCoefficients (true);
    seg.setModelType (pcl::SACMODEL_PLANE);
    seg.setMethodType (pcl::SAC_RANSAC);
    seg.setMaxIterations (100);
    seg.setDistanceThreshold (0.2);
    seg.setInputCloud (cloud_filtered);

    // Segment the largest planar component from the remaining cloud
    seg.setInputCloud (cloud_filtered);
    seg.segment (*inliers, *coefficients);

    // Extract the planar inliers from the input cloud
    pcl::ExtractIndices<pcl::PointXYZI> extract;
    extract.setInputCloud (cloud_filtered);
    extract.setIndices (inliers);
    extract.setNegative (false);

    // Get the points associated with the planar surface
    extract.filter (*cloud_plane);
    std::cout << str << " :PointCloud representing the planar component: " << cloud_plane->points.size () << " data points." << std::endl;

    // Remove the planar inliers, extract the rest
    extract.setNegative (true);
    extract.filter (*cloud_f);
    *cloud_filtered = *cloud_f;
    std::cout << str <<" >> Done: " << tt.toc () << " ms\n";

    std::cout << str << " :Building kdTree and finding all clusters (Euclidian cluster extraction)",tt.tic ();
    // Creating the KdTree object for the search method of the extraction
    pcl::search::KdTree<pcl::PointXYZI>::Ptr tree (new pcl::search::KdTree<pcl::PointXYZI>);
    tree->setInputCloud (cloud_filtered);

    std::vector<pcl::PointIndices> cluster_indices;
    pcl::EuclideanClusterExtraction<pcl::PointXYZI> ec;
    ec.setClusterTolerance (0.25); // 0.02 = 2cm
    ec.setMinClusterSize (150);
    ec.setMaxClusterSize (15000);
    ec.setSearchMethod (tree);
    ec.setInputCloud (cloud_filtered);
    ec.extract (cluster_indices);

    int j = 1;
    for (std::vector<pcl::PointIndices>::const_iterator it = cluster_indices.begin (); it != cluster_indices.end (); ++it)
    {
      pcl::PointCloud<pcl::PointXYZI>::Ptr cloud_cluster (new pcl::PointCloud<pcl::PointXYZI>);
      for (std::vector<int>::const_iterator pit = it->indices.begin (); pit != it->indices.end (); ++pit)
        cloud_cluster->points.push_back (cloud_filtered->points[*pit]); //*
      cloud_cluster->width = cloud_cluster->points.size ();
      cloud_cluster->height = 1;
      cloud_cluster->is_dense = true;

      //UNCOMMENT TO ADD WHITEBOXES
      /*pcl::MomentOfInertiaEstimation <pcl::PointXYZI> feature_extractor;
      feature_extractor.setInputCloud (cloud_cluster);
      feature_extractor.compute ();

      std::vector <float> moment_of_inertia;
      std::vector <float> eccentricity;
      pcl::PointXYZ min_point_OBB;
      pcl::PointXYZ max_point_OBB;
      pcl::PointXYZ position_OBB;
      Eigen::Matrix3f rotational_matrix_OBB;
      float major_value, middle_value, minor_value;
      Eigen::Vector3f major_vector, middle_vector, minor_vector;
      Eigen::Vector3f mass_center;

      feature_extractor.getMomentOfInertia (moment_of_inertia);
      feature_extractor.getEccentricity (eccentricity);
      feature_extractor.getOBB (min_point_OBB, max_point_OBB, position_OBB, rotational_matrix_OBB);
      feature_extractor.getEigenValues (major_value, middle_value, minor_value);
      feature_extractor.getEigenVectors (major_vector, middle_vector, minor_vector);
      feature_extractor.getMassCenter (mass_center);

      Eigen::Vector3f position (position_OBB.x, position_OBB.y, position_OBB.z);
      Eigen::Quaternionf quat (rotational_matrix_OBB);
      //viewer->addCube (position, quat, max_point_OBB.x - min_point_OBB.x, max_point_OBB.y - min_point_OBB.y, max_point_OBB.z - min_point_OBB.z, ("id" + j));
      //viewer->addText3D ("Yoda" +j, position_OBB, 1.0, 1.0, 1.0,1.0, "id" + j ,0);
      //viewer->addText3D ("ID:"+j, position_OBB, 1.0, 1.0, 1.ou, 1.0);
      
      //std::cout << "PointCloud representing the Cluster: " << cloud_cluster->points.size () << " data points." << std::endl;
      //std::stringstream ss;
      //ss << "cloud_cluster_" << j << ".pcd";
      //writer.write<pcl::PointXYZI> (ss.str (), *cloud_cluster, false); /*/
      j++;
    }

    std::cout << str << " >> Done: " << tt.toc () << " ms\n";
    std::cout << str << " :found: " << j << " clusters." << endl;
  }
  
  // COMMENT 3DVIEWER BEFORE PUSHING TO ODROID

  // ----------------------------------------------------------------------------------------------------------
  // -----Open 3D viewer and add point cloud-----
  
  // boost::shared_ptr<pcl::visualization::PCLVisualizer> viewer (new pcl::visualization::PCLVisualizer ("3D Viewer"));
  // viewer->setBackgroundColor (0, 0, 0);
  // viewer->addPointCloud<pcl::PointXYZI> (cloud_filtered, "Cloud with boxes");
  // viewer->setPointCloudRenderingProperties (pcl::visualization::PCL_VISUALIZER_POINT_SIZE, 1, "Cloud with boxes");
  // viewer->addCoordinateSystem (1.0);
  // viewer->initCameraParameters ();
  //------------------------------------------------------------------------------------------------------------
  
  // COMMENT 3DVIEWER ABOVE BEFORE PUSHING TO ODROIDS

<<<<<<< HEAD
  int j = 1;
  for (std::vector<pcl::PointIndices>::const_iterator it = cluster_indices.begin (); it != cluster_indices.end (); ++it)
  {
    pcl::PointCloud<pcl::PointXYZI>::Ptr cloud_cluster (new pcl::PointCloud<pcl::PointXYZI>);
    for (std::vector<int>::const_iterator pit = it->indices.begin (); pit != it->indices.end (); ++pit)
      cloud_cluster->points.push_back (cloud_filtered->points[*pit]); //*
    cloud_cluster->width = cloud_cluster->points.size ();
    cloud_cluster->height = 1;
    cloud_cluster->is_dense = true;

    //UNCOMMENT TO ADD WHITEBOXES
    /*pcl::MomentOfInertiaEstimation <pcl::PointXYZI> feature_extractor;
    feature_extractor.setInputCloud (cloud_cluster);
    feature_extractor.compute ();

    std::vector <float> moment_of_inertia;
    std::vector <float> eccentricity;
    pcl::PointXYZ min_point_OBB;
    pcl::PointXYZ max_point_OBB;
    pcl::PointXYZ position_OBB;
    Eigen::Matrix3f rotational_matrix_OBB;
    float major_value, middle_value, minor_value;
    Eigen::Vector3f major_vector, middle_vector, minor_vector;
    Eigen::Vector3f mass_center;

    feature_extractor.getMomentOfInertia (moment_of_inertia);
    feature_extractor.getEccentricity (eccentricity);
    feature_extractor.getOBB (min_point_OBB, max_point_OBB, position_OBB, rotational_matrix_OBB);
    feature_extractor.getEigenValues (major_value, middle_value, minor_value);
    feature_extractor.getEigenVectors (major_vector, middle_vector, minor_vector);
    feature_extractor.getMassCenter (mass_center);

    Eigen::Vector3f position (position_OBB.x, position_OBB.y, position_OBB.z);
    Eigen::Quaternionf quat (rotational_matrix_OBB);
    //viewer->addCube (position, quat, max_point_OBB.x - min_point_OBB.x, max_point_OBB.y - min_point_OBB.y, max_point_OBB.z - min_point_OBB.z, ("id" + j));
    //viewer->addText3D ("Yoda" +j, position_OBB, 1.0, 1.0, 1.0,1.0, "id" + j ,0);
    //viewer->addText3D ("ID:"+j, position_OBB, 1.0, 1.0, 1.0, 1.0);
    
    //std::cout << "PointCloud representing the Cluster: " << cloud_cluster->points.size () << " data points." << std::endl;
    //std::stringstream ss;
    //ss << "cloud_cluster_" << j << ".pcd";
    //writer.write<pcl::PointXYZI> (ss.str (), *cloud_cluster, false); /*/
    j++;
  }

  std::string pi = "pi is " + std::to_string(3.1415926);

  std::cerr << ">> Done: " << tt.toc () << " ms\n";

  std::cout << "found: " << j << " clusters." << endl;
=======
  std::cerr << ">> All files done: " << pT.toc () << " ms\n";
>>>>>>> 7675a631730c3f69fae22f9d2df7b4060bc47f22
  
  //COMMENT THIS WHEN RUNNING ON ODROID TO REMOVE 3DVIEWER
  
  //while (!viewer->wasStopped ())
  //{
  //  viewer->spinOnce (100);
  //  boost::this_thread::sleep (boost::posix_time::microseconds (100000));
  //}
  //COMMENT THIS ABOVE WHEN RUNNING ON ODROID TO REMOVE 3DVIEWER
  
  return (0);
}