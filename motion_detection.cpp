// boost
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/filesystem.hpp>
// opencv
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/video.hpp>
// C
#include <stdio.h>
// C++
#include <iostream>
#include <sstream>
#include <string>

using namespace cv;
using namespace std;
namespace po = boost::program_options;
namespace pt = boost::posix_time;
namespace fs = boost::filesystem;

#define MAX_FILE_NAME 

/**
 * @function main
 */
int main(int argc, char* argv[])
{
  int keyboard = 0;
  Mat frame;
  Mat fgMaskMOG2;
  Ptr<BackgroundSubtractor> pMOG2 = createBackgroundSubtractorMOG2(1000, 400.0, false);
  Mat kernel = getStructuringElement(MORPH_RECT, Size(3,3));
  int countContours = 0;


  int device;
  string fourcc;
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help", "help message")
    ("view,x", "show opencv windows")
    ("device,d", po::value<int>(&device)->default_value(0), "video device")
    ("width,w", po::value<int>(), "capture width")
    ("height,h", po::value<int>(), "capture height")
    ("fps,f", po::value<int>(), "capture frame rate")
    ("fourcc,4", po::value< string >(&fourcc)->default_value("YUYV"), "capture fourcc")
    ("thresold,t", po::value<int>(&device)->default_value(0), "minimum moved pixels to save an image")
    ;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);    

  if (vm.count("help")) {
    cout << desc << "\n";
    exit(EXIT_FAILURE);
  }

  if (vm.count("view")) {
    //create GUI windows
    namedWindow("Frame");
    namedWindow("FG Mask MOG 2");
  }
  
  //create the capture object
  VideoCapture capture(device);
  if(!capture.isOpened()){
    //error in opening the video input
    cerr << "Unable to open camera : /dev/video" << device << endl;
    exit(EXIT_FAILURE);
  }

  if (vm.count("fps")) {
    capture.set(CAP_PROP_FPS, vm["fps"].as<int>());
  }
  if (vm.count("width")) {
    capture.set(CAP_PROP_FRAME_WIDTH, vm["width"].as<int>());
  }
  if (vm.count("height")) {
    capture.set(CAP_PROP_FRAME_HEIGHT, vm["height"].as<int>());
  }
  int ifourcc = VideoWriter::fourcc(fourcc[0], fourcc[1], fourcc[2], fourcc[3]);
  capture.set(CAP_PROP_FOURCC, ifourcc);
  
  cout << "motion: cam : width=" << capture.get(CAP_PROP_FRAME_WIDTH) <<
    ", height=" << capture.get(CAP_PROP_FRAME_HEIGHT) <<
    ", fps=" << capture.get(CAP_PROP_FPS) <<
    endl;
  
  int thresold = vm["thresold"].as<int>();
  int num_frames = 0;
  time_t start, end;
  ptime now;

  time_facet *facet_filename = new time_facet("motion %Y-%m-%d %H:%M:%s.jpg");
  std::string filename;
  std::stringstream stream_filename;
  stream_filename.imbue(std::locale(std::locale::classic(), facet_filename));

  time_facet *facet_path = new time_facet("%Y-%m-%d");
  std::string path;
  std::stringstream stream_path;
  stream_path.imbue(std::locale(std::locale::classic(), facet_path));

  time(&start);
  //read input data. ESC or 'q' for quitting
  while( (char)keyboard != 'q' && (char)keyboard != 27 ){
    //read the current frame
    if(!capture.read(frame)) {
      cerr << "motion: unable to read next frame." << endl;
      cerr << "motion: exiting..." << endl;
      exit(EXIT_FAILURE);
    }
    now = microsec_clock::local_time();
    
    num_frames += 1;
    if (num_frames % 10 == 0) {
      time(&end);
      double fps  = num_frames / difftime (end, start);
      cout << "motion: estimated frame rate : \"" << fps << "\" fps" << endl;
    }
    
    //update the background model
    pMOG2->apply(frame, fgMaskMOG2);

    // Clean foreground from noise
    morphologyEx(fgMaskMOG2, fgMaskMOG2, MORPH_OPEN, kernel);

    int num_motion_pixels = countNonZero(fgMaskMOG2);
    if (num_motion_pixels > thresold) {
      stream_filename.str("");
      stream_filename << now;
      filename = stream_filename.str();

      stream_path.str("");
      stream_path << now;
      path = stream_path.str();

      boost::filesystem::path p = boost::filesystem::path(path);
      if (boost::filesystem::create_directory(p)) {
	cout << "motion: path: \"" << path << "\" created!" << endl;
      }
      assert(boost::filesystem::exists(p));

      std::string file = (p / filename).string();
      imwrite(file, frame);
      cout << "motion: file \"" << file << "\" saved" << endl;
      cout << "motion: moved pixels \"" << num_motion_pixels << "\" detected" << endl;
    }

    if (vm.count("view")) {
      // Find contours
      vector< vector< Point > > contours;
      findContours(fgMaskMOG2.clone(), contours, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE);

      for(std::vector<int>::size_type i = 0; i != contours.size(); i++) {
        drawContours(frame, contours, i, Scalar(0, 0, 255));
      }
  
      //show the current frame and the fg masks
      imshow("Frame", frame);
      imshow("FG Mask MOG 2", fgMaskMOG2);
      //get the input from the keyboard
      keyboard = waitKey( 30 );
    }
  }

  //delete capture object
  capture.release();
  destroyAllWindows();
  return EXIT_SUCCESS;
}
