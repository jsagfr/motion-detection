// qt5 core
#include <QDateTime>
#include <QCommandLineParser>
#include <QDir>
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

int main(int argc, char* argv[])
{
  int keyboard = 0;
  Mat frame;
  Mat fgMaskMOG2;
  Ptr<BackgroundSubtractor> pMOG2 = createBackgroundSubtractorMOG2(1000, 400.0, false);
  Mat kernel = getStructuringElement(MORPH_RECT, Size(3,3));
  int countContours = 0;

  QCoreApplication app(argc, argv);
  QCoreApplication::setApplicationName("motion-detection");
  QCoreApplication::setApplicationVersion("0.1");

  QCommandLineParser parser;
  parser.setApplicationDescription("Motion Detection");
  parser.addHelpOption();
  parser.addVersionOption();

  QCommandLineOption viewOption(QStringList() << "x" << "view",
                                "show opencv windows.");
  parser.addOption(viewOption);

  // An option with a value
  QCommandLineOption deviceOption(QStringList() << "d" << "device",
                                  "video device <cam number>.",
                                  "device", "0");
  parser.addOption(deviceOption);

  QCommandLineOption widthOption(QStringList() << "w" << "width",
                                 "capture <width> pixels.",
                                 "width", "800");
  parser.addOption(widthOption);

  QCommandLineOption heightOption(QStringList() << "H" << "height",
                                  "capture <height> pixels.",
                                  "height", "600");
  parser.addOption(heightOption);

  QCommandLineOption fpsOption(QStringList() << "f" << "fps",
                               "capture <frame rate>.",
                               "fps", "5");
  parser.addOption(fpsOption);

  QCommandLineOption fourccOption(QStringList() << "4" << "fourcc",
                                  "fourcc capture <value>.",
                                  "fourcc", "YUYV");
  parser.addOption(fourccOption);

  QCommandLineOption thresoldOption(QStringList() << "t" << "thresold",
                                    "thresold <pixels moved> to save image.",
                                    "thresold", "0");
  parser.addOption(thresoldOption);
  // Process the actual command line arguments given by the user
  parser.process(app);

  bool view = parser.isSet(viewOption);
  bool ok;
  int device = parser.value(deviceOption).toInt(&ok, 10);
  if (!ok) {
    cerr << "device parameter \"" << parser.value(deviceOption) <<"\" invalid" << endl;
    exit(EXIT_FAILURE);
  }
  int width = parser.value(widthOption).toInt(&ok, 10);
  if (!ok) {
    cerr << "width parameter \"" << parser.value(widthOption) <<"\" invalid" << endl;
    exit(EXIT_FAILURE);
  }
  int height = parser.value(heightOption).toInt(&ok, 10);
  if (!ok) {
    cerr << "height parameter \"" << parser.value(heightOption) <<"\" invalid" << endl;
    exit(EXIT_FAILURE);
  }
  int fps = parser.value(fpsOption).toInt(&ok, 10);
  if (!ok) {
    cerr << "fps parameter \"" << parser.value(fpsOption) <<"\" invalid" << endl;
    exit(EXIT_FAILURE);
  }
  std::string fourcc = parser.value(fourccOption).toStdString();
  int thresold = parser.value(thresoldOption).toInt(&ok, 10);
  if (!ok) {
    cerr << "thresold parameter \"" << parser.value(thresoldOption) <<"\" invalid" << endl;
    cerr << "thresold parameter error" << endl;
    exit(EXIT_FAILURE);
  }
  

  if (view) {
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

  capture.set(CAP_PROP_FPS, fps);
  capture.set(CAP_PROP_FRAME_WIDTH, width);
  capture.set(CAP_PROP_FRAME_HEIGHT, height);
  int ifourcc = VideoWriter::fourcc(fourcc[0], fourcc[1], fourcc[2], fourcc[3]);
  capture.set(CAP_PROP_FOURCC, ifourcc);
  
  cout << "motion: cam : width=" << capture.get(CAP_PROP_FRAME_WIDTH) <<
    ", height=" << capture.get(CAP_PROP_FRAME_HEIGHT) <<
    ", fps=" << capture.get(CAP_PROP_FPS) <<
    endl;
  
  int num_frames = 0;
  // time_t start, end;
  QDir dir = QDir::current();
  //read input data. ESC or 'q' for quitting
  while( (char)keyboard != 'q' && (char)keyboard != 27 ){
    //read the current frame
    if(!capture.read(frame)) {
      cerr << "motion: unable to read next frame." << endl;
      cerr << "motion: exiting..." << endl;
      exit(EXIT_FAILURE);
    }
    QDateTime now(QDateTime::currentDateTime());
    
    // num_frames += 1;
    // if (num_frames % 10 == 0) {
    //   time(&end);
    //   double fps  = num_frames / difftime (end, start);
    //   cout << "motion: estimated frame rate : \"" << fps << "\" fps" << endl;
    // }
    
    //update the background model
    pMOG2->apply(frame, fgMaskMOG2);

    // Clean foreground from noise
    morphologyEx(fgMaskMOG2, fgMaskMOG2, MORPH_OPEN, kernel);

    int num_motion_pixels = countNonZero(fgMaskMOG2);
    if (num_motion_pixels > thresold) {
      QString dirname = now.toString("yyyy-MM-dd");
      dir.mkpath(dirname);

      QDir file(dirname + "/motion " + now.toString("yyyy-MM-dd HH:mm:ss.zzz") + ".jpg");
      std::string filename = file.path().toStdString();

      imwrite(filename, frame);
      cout << "motion: file \"" << filename << "\""
           << ", folder \"" << dirname.toStdString() << "\""
           << ", pixels \"" << num_motion_pixels << "\"" << endl;
    }

    if (view) {
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
