/*
=====
Copyright (C) 2011 Massachusetts Institute of Technology


This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
======
*/


#include "OcpiWorker.h"
#include "OcpiPValue.h"
#include "OcpiOsMisc.h"
#include "OcpiOsMutex.h"
#include "OcpiUtilIomanip.h"
#include "OcpiUtilException.h"
#include "OcpiContainerPort.h"
#include "OcpiOsThreadManager.h"
#include "OcpiContainerInterface.h"

#include "DemoUtil.h"
#include "DemoWorkerFacade.h"

#include "highgui.h"
#include "cv.h"

int main ( int argc, char* argv [ ] )
{
	if(argc != 2) {
		std::cout << std::endl
			<< "Usage: ./feature_detection <image_name>\n"
			<< std::endl;
		return 0;
	}

	try
	{
		( void ) argc;
		( void ) argv;

		/* ---- Load the image from file (grayscale) -------------- */
		IplImage* inImg = cvLoadImage( argv[1], 0 );
		// Create image with 8U pixel depth, single color channel
		IplImage* img = cvCreateImage(
				cvSize(inImg->width, inImg->height),
				IPL_DEPTH_8U, 1
		);
		// Convert
		cvConvertScale(inImg, img);
		cvNamedWindow( "Input", CV_WINDOW_AUTOSIZE );
		cvShowImage( "Input", img );

		// Create an image for the output
		IplImage* outImg = cvLoadImage( argv[1] );
		cvNamedWindow( "Output", CV_WINDOW_AUTOSIZE );

		/* ---- Create the shared RCC container and application -------------- */
		OCPI::Container::Interface* rcc_container ( Demo::get_rcc_interface ( ) );

		// Holds RCC worker interfaces passed to the RCC dispatch thread
		std::vector<OCPI::Container::Interface*> interfaces;
		interfaces.push_back ( rcc_container );

		OCPI::Container::Application*
			rcc_application ( rcc_container->createApplication ( ) );

		// Holds facades for group operations
		std::vector<Demo::WorkerFacade*> facades;

		/* ---- Create the workers --------------------------------- */
    // good_features_to_track
		Demo::WorkerFacade features_worker (
        "GoodFeaturesToTrack (RCC)",
				rcc_application,
				Demo::get_rcc_uri ( "good_features_to_track" ).c_str ( ),
				"good_features_to_track" );

		OCPI::Util::PValue features_worker_pvlist[] = {
			OCPI::Util::PVULong("height", img->height),
			OCPI::Util::PVULong("width", img->width),
      OCPI::Util::PVULong("max_corners", 50),
      OCPI::Util::PVDouble("quality_level", 0.03),
      OCPI::Util::PVDouble("min_distance", 5.0),
			OCPI::Util::PVEnd
		};
		features_worker.set_properties( features_worker_pvlist );
		facades.push_back ( &features_worker );

		OCPI::Container::Port
			&featuresOut = features_worker.port("out"),
			&featuresIn = features_worker.port("in");

    // min_eigen_val
		Demo::WorkerFacade min_worker (
        "MinEigenVal (RCC)",
				rcc_application,
				Demo::get_rcc_uri ( "min_eigen_val" ).c_str ( ),
				"min_eigen_val" );

		OCPI::Util::PValue min_worker_pvlist[] = {
			OCPI::Util::PVULong("height", img->height),
			OCPI::Util::PVULong("width", img->width),
			OCPI::Util::PVEnd
		};
		min_worker.set_properties( min_worker_pvlist );
		facades.push_back ( &min_worker );

		OCPI::Container::Port
			&minOut = min_worker.port("out"),
			&minIn = min_worker.port("in");

    // corner_eigen_vals_vecs
		Demo::WorkerFacade corner_worker (
        "CornerEigenValsVecs (RCC)",
				rcc_application,
				Demo::get_rcc_uri ( "corner_eigen_vals_vecs" ).c_str ( ),
				"corner_eigen_vals_vecs" );

		OCPI::Util::PValue corner_worker_pvlist[] = {
			OCPI::Util::PVULong("height", img->height),
			OCPI::Util::PVULong("width", img->width),
			OCPI::Util::PVEnd
		};
		corner_worker.set_properties( corner_worker_pvlist );
		facades.push_back ( &corner_worker );

		OCPI::Container::Port
			&cornerOut = corner_worker.port("out"),
			&cornerIn = corner_worker.port("in");

		/* ---- Create connections --------------------------------- */

    featuresIn.connect( minOut );
    minIn.connect( cornerOut );

		// Set external ports
		OCPI::Container::ExternalPort
			&myOut = cornerIn.connectExternal("aci_out"),
			&myIn = featuresOut.connectExternal("aci_in");

    printf(">>> DONE CONNECTING!\n");

		/* ---- Start all of the workers ------------------------------------- */
		std::for_each ( facades.rbegin ( ),
				facades.rend ( ),
				Demo::start );

		// Note: We must call dispatch before the first call to 
		// ExternalPort::getBuffer or else it will seg fault
		std::for_each ( interfaces.begin(),
				interfaces.end(),
				Demo::dispatch );

    printf(">>> DONE STARTING!\n");

		// Output info
		uint8_t *odata;
		uint32_t olength;

		// Input info
		uint8_t *idata;
		uint32_t ilength;
		uint8_t opcode = 0;
		bool isEndOfData = false;

    // Set output data
    OCPI::Container::ExternalBuffer* myOutput = myOut.getBuffer(odata, olength);
    memcpy(odata, img->imageData, img->height * img->width);
    myOutput->put(0, img->height * img->width, false);

    std::cout << "My output buffer is size " << olength << std::endl;

    // Call dispatch so the worker can "act" on its input data
    std::for_each ( interfaces.begin(), interfaces.end(), Demo::dispatch );
    std::for_each ( interfaces.begin(), interfaces.end(), Demo::dispatch );
    std::for_each ( interfaces.begin(), interfaces.end(), Demo::dispatch );

    // Get input data
    OCPI::Container::ExternalBuffer* myInput = myIn.getBuffer(opcode, idata,
      ilength, isEndOfData);

    std::cout << "My input buffer is size " << ilength << std::endl;

    myInput->release();
    // Mark features
    size_t ncorners = ilength / (2 * sizeof(float));
    float *corners = (float *) idata;
    std::cout << "My corners " << ncorners << std::endl;
    for( int i = 0; i < ncorners; i++ ) {
      float x = corners[2*i];
      float y = corners[2*i+1];
      CvPoint p = cvPoint( cvRound(x), cvRound(y) );
      CvScalar color = CV_RGB(255, 0, 0);
      cvCircle( outImg, p, 5, color, 2 );
    }

		// Show image
		cvShowImage( "Output", outImg );

		std::cout << "\nOpenOCPI application is done\n" << std::endl;

		// Save image
		cvSaveImage("output_image.jpg", outImg);

		// Cleanup
		cvWaitKey(0);
		cvReleaseImage( &img );
		cvReleaseImage( &outImg );
		cvDestroyWindow( "Input" );
		cvDestroyWindow( "Output" );
	}
	catch ( const std::string& s )
	{
		std::cerr << "\n\nException(s): " << s << "\n" << std::endl;
	}
	catch ( const OCPI::Container::ApiError& a )
	{
		std::cerr << "\nException(a): rc="
							<< a.getErrorCode ( )
							<< " : "
							<< a.getAuxInfo ( )
							<< "\n"
							<< std::endl;
  }
  catch ( const OCPI::Util::EmbeddedException& e )
  {
    std::cerr << "\nException(e): rc="
              << e.getErrorCode ( )
              << " : "
              << e.getAuxInfo ( )
              << "\n"
              << std::endl;
  }
  catch ( std::exception& g )
  {
    std::cerr << "\nException(g): "
              << typeid( g ).name( )
              << " : "
              << g.what ( )
              << "\n"
              << std::endl;
  }
  catch ( ... )
  {
    std::cerr << "\n\nException(u): unknown\n" << std::endl;
  }

  return 0;
}


