
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
	if(argc != 3) {
		std::cout << std::endl
			<< "Usage: ./feature_detection <image_name> <worker_name>\n"
			<< std::endl;
		return 0;
	}
	char* worker_name = argv[2];

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
		/*
		cvWaitKey(0);
		return 0;
		*/
		/*
		// Save smaller version
		IplImage* smallImg = cvCreateImage(
				cvSize(img->width / 2, img->height / 2),
				img->depth, img->nChannels
		);
		cvResize(img, smallImg);
		cvSaveImage("boston_small.jpg", smallImg);
		*/

		// Create an image for the output (grayscale)
		IplImage* outImg = cvCreateImage(
				cvSize(img->width, img->height),
				img->depth, img->nChannels
		);
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

		/* ---- Create the worker --------------------------------- */
		Demo::WorkerFacade worker ( worker_name,
				rcc_application,
				Demo::get_rcc_uri ( worker_name ).c_str ( ),
				worker_name );

		// Set properties
		OCPI::Util::PValue worker_pvlist[] = {
			OCPI::Util::PVULong("height", img->height),
			OCPI::Util::PVULong("width", img->width),
      // TODO - investigate PVDouble?
      OCPI::Util::PVULong("low_thresh", 10), // Canny
      OCPI::Util::PVULong("high_thresh", 100), // Canny
			OCPI::Util::PVEnd
		};
		worker.set_properties( worker_pvlist );
		facades.push_back ( &worker );

		// Set ports
		OCPI::Container::Port
			&wOut = worker.port("out"),
			&wIn = worker.port("in");

		// Set external ports (need 3 buffers for out)
		OCPI::Container::ExternalPort
			&myOut = wIn.connectExternal("aci_out"),
			&myIn = wOut.connectExternal("aci_in");

		/* ---- Start all of the workers ------------------------------------- */
		std::for_each ( facades.rbegin ( ),
				facades.rend ( ),
				Demo::start );

		// Note: We must call dispatch before the first call to 
		// ExternalPort::getBuffer or else it will seg fault
		std::for_each ( interfaces.begin(),
				interfaces.end(),
				Demo::dispatch );

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
    memcpy(odata, (uint8_t *) img->imageData, img->height * img->widthStep);
    myOutput->put(0, img->height * img->widthStep, false);

    std::cout << "My output buffer is size " << olength << std::endl;

    // Call dispatch so the worker can "act" on its input data
    std::for_each ( interfaces.begin(),
        interfaces.end(),
        Demo::dispatch );

    // Get input data
    OCPI::Container::ExternalBuffer* myInput = myIn.getBuffer(opcode, idata,
      ilength, isEndOfData);

    std::cout << "My input buffer is size " << ilength << std::endl;
    std::cout << "The contents of the input buffer is ";

    myInput->release();
    memcpy((uint8_t *) outImg->imageData, idata, outImg->height * outImg->widthStep);

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


