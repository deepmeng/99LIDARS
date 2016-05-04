#include <pcl/console/time.h>
#include <string>
#include <vector>
#include <sstream>
#include <mpi.h>
#include <algorithm>    // std::sort

#include "filter.h"
#include "segmentation.h"


using namespace pcl;
using namespace std;

static int numprocs;

bool less_vectors(const vector<float>& a,const vector<float>& b) {
   return a.size() < b.size();
}

/*************************************************************************************
*		Main
**************************************************************************************/
int main(int argc, char **argv) {
	int my_rank = 0;

	bool dbscan = false;
	int nth_point = 5; // five is default
	double eps = 0.6; // epsilon for clustering default 0.6 for the
	int minCl = 30;

	for (int i = 1; i < argc; i++) { 
        if (i + 1 != argc) // Check that we haven't finished parsing already
            if(std::strcmp(argv[i], "-d") == 0) {
                dbscan = true;
            } else if(std::strcmp(argv[i], "-n") == 0){
					//nth_point = atoi(argv[i+1]);
					sscanf(argv[i+1], "%i", &nth_point);
			} else if(std::strcmp(argv[i], "-e") == 0){
				eps = atof(argv[i+1]);
			} else if(std::strcmp(argv[i], "-m") == 0){
				minCl = atoi(argv[i+1]);                   
        }
        std::cout << argv[i] << " ";
	}

	  //cout << endl << "Arg, n: " << nth_point << " eps: " << eps << " minCl: " << minCl << endl;

	// MPI initializations
	MPI_Status status;
	MPI_Init (&argc, &argv);
	MPI_Comm_size (MPI_COMM_WORLD, &numprocs);
	MPI_Comm_rank (MPI_COMM_WORLD, &my_rank);


/*******************************************************************************************
*		Master runs this code
********************************************************************************************/
	if(my_rank == 0){ 

	  int read_file;
	  pcl::console::TicToc tt;
	  tt.tic();	  
	  //std::vector<std::vector<float> > floats;
	  int m_tag = 0; // MPI message tag

	  std::string infile = "../../BinAndTxt/0000000002.bin";

	  // Read file and create 8 point clouds
	  // Nth_point will be kept from the data e.g. 3, every third point will be used
	  Filters filt;
	  filt.read_file(infile, nth_point);
	  filt.filter_and_slice();

	 read_file = tt.toc();

	 std::sort(filt.floats.begin(),filt.floats.end(),less_vectors);

	int sectors = 8;
	
	 for(int i = 0; i < sectors ; i++){ 
	   int bsize = filt.floats.at(i).size();
	   cout << "floats: " << i << "\t" << bsize;
	 }

	
	 tt.tic(); // Distributing, processing, and cathering
	for(int i = 0; i < sectors ; i++){ 
	   int bsize = filt.floats.at(i).size();
	   MPI_Send(&bsize, 1, MPI_INT, (i+1), m_tag, MPI_COMM_WORLD);
	   float *f = &filt.floats.at(i)[0];
	   MPI_Send(f, bsize, MPI_FLOAT, (i+1), m_tag, MPI_COMM_WORLD);
	}
	 int sending = tt.toc(); 
	
	for(int i = 0; i < sectors ; i++){ 
		int number_amount;
		MPI_Status status;
		MPI_Probe(MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
		MPI_Get_count(&status, MPI_FLOAT, &number_amount);

		// Allocate a buffer to hold the incoming numbers
		float* number_buf = (float*)malloc(sizeof(float) * number_amount);
		// Now receive the message with the allocated buffer
		MPI_Recv(number_buf, number_amount, MPI_FLOAT, status.MPI_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);  
		free(number_buf);
		cout << "Master received " << number_amount << " values from " << status.MPI_SOURCE << endl; 
	}
	int processing = tt.toc();
	cout << "Read file and filter: " << read_file << " ms" << endl;
	cout << "Sending data-loop: " << sending << endl;
	cout << "Distributing, processing, gathering:" << processing << " ms" << endl;
	cout << "Total: " << (read_file + processing) <<  endl;

	
/********************************************************************************************************
*		Workers run this code
********************************************************************************************************/
}else if(my_rank > 0){ 

	pcl::console::TicToc tt;


   float buff [50000]; 
   int count;
   MPI_Recv(&count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
   MPI_Recv(&buff, count, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  
 	Segmentation seg;
 	seg.build_cloud(buff, count);

 	cout << "Manage to build cloud! " << seg.cloud.size() << " points." << endl;
 	seg.ransac(0.25, 100); // double Threshhold, int max_number_of_iterations

 	float buffer[200];
 	double eps = 0.6;
 	int minCl = 30;
 	
 	int bsize = seg.euclidian(buffer, eps, minCl); // Returns size of float buffer

	//Send back boxes of found clusters to master
	int root = 0;
	//MPI_Send(&buffer, bsize, MPI_FLOAT, root, 0, MPI_COMM_WORLD); // used with db scan
	MPI_Send(&buffer, bsize  , MPI_FLOAT, root, 0, MPI_COMM_WORLD);// Used with euclidian 
}
//******************************************************************************************************
// End MPI
MPI_Finalize ();
return 0;
}

