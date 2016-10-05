/******************************************************************************************
  Date:    12.08.2016
  Author:  Nathan Greco (Nathan.Greco@gmail.com)

  Project:
      DAPrototype: Driver Assist Prototype (Raspberry Pi Application)

  Description:
      This project is an attempt to create a standalone, windshield mounted driver assist
      unit with LDW, FCW, and dashcam functionality.  Future versions may also incorporate
      road sign recognition, stop-light change notification, forward car pull-ahead, and
	  bluetooth inteface.

  Target Hardware:
      Raspberry Pi (v3) with RPi (v2) 5MP camera, LidarLite (v3) LIDAR rangefinder, and
      adafruit Ultimate GPS.

  Target Software platform:
      Debian disto with X11 GUI (GTK2.0)

  3rd Party Libraries:
      OpenCV 3.1.0 -> Compiled with OpenGL support, www.opencv.org
      Raspicam ?? -> , ??

  Other notes:
      Style is following the Google C++ styleguide

  History:
      Date         Author      Description
-------------------------------------------------------------------------------------------
      12.08.2016   N. Greco    Initial creation
******************************************************************************************/


//Standard libraries
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <cstdlib>
#include <mutex>
#include <atomic>

//DAPrototype source files
#include "display_handler.h"
#include "gpio_handler.h"
//#include "gps_polling.h"
#include "image_capturer.h"
#include "image_editor.h"
#include "image_processor.h"
#include "lane_detect_processor.h"
#include "lidar_polling.h"
#include "pace_setter_class.h"
#include "process_values_class.h"
#include "video_writer.h"
#include "xml_reader.h"

//3rd party libraries
#include "opencv2/opencv.hpp"

int main()
{
	std::cout << "Program launched, starting log file..." << std::endl;
	//Quick and dirty log file
	std::ofstream out("log.txt");
    std::streambuf *coutbuf = std::cout.rdbuf();
    std::cout.rdbuf(out.rdbuf());
	
    //Check XML Properties
	if (settings::kreadsuccess < 0) {
        std::cout << "XML reading failed, using defaults." << std::endl;
    } else {
        std::cout << "XML reading successful!" << std::endl;
    }

	//Create shared resources
	std::atomic<bool> exitsignal{ false };
	std::atomic<bool> shutdownsignal{ false };
    cv::Mat captureimage;
	std::mutex capturemutex;
	cv::Mat displayimage;
	std::mutex displaymutex;
	ProcessValues processvalues;
	
	//Start threads

	//Start image capture thread
    std::thread t_imagecapture( CaptureImageThread,
	                            &captureimage,
								&capturemutex,
								&exitsignal );
	//Start image processor thread
    std::thread t_imageprocessor( ProcessImageThread,
								  &captureimage,
								  &capturemutex,
								  &processvalues,
								  &exitsignal );
	//Start display thread
	std::thread t_displayupdate( DisplayUpdateThread,
								 &displayimage,
								 &displaymutex,
								 &exitsignal );
	//Start imeage editor thread
    std::thread t_imeageeditor( ImageEditorThread,
								&captureimage,
								&capturemutex,
								&displayimage,
								&displaymutex,
								&processvalues,
								&exitsignal );
	//Start video writer thread
    std::thread t_videowriter( VideoWriterThread,
							   &captureimage,
							   &capturemutex,
							   &displayimage,
							   &displaymutex,
							   &exitsignal );
	//Start GPS poling thread
//	std::thread t_gpspolling( GpsPollingThread,
//							  &processvalues,
//							  &exitsignal );
	//Start LIDAR polling thread
	std::thread t_lidarpolling( LidarPolingThread,
								&processvalues,
								&exitsignal );
	//Start GPIO thread
	std::thread t_gpiohandler( GpioHandlerThread,
							   &processvalues,
							   &exitsignal,
							   &shutdownsignal );
	
    //Test pace setter class!
    PaceSetter mypacesetter( 10, "main" );
	while( !exitsignal ){
		//Should check all threads still running
		mypacesetter.SetPace();
	}

    //Handle all the threads
	t_videowriter.join();
	t_imageprocessor.join();
	t_lidarpolling.join();
//	t_gpspolling.join();
	t_imeageeditor.join();
	t_imagecapture.join();
	t_displayupdate.join();
	shutdownsignal = true;
	t_gpiohandler.join();
    std::cout.rdbuf(coutbuf); //reset to standard output again
	std::cout << "Program exited gracefully!"  << std::endl;

    return 0;
}