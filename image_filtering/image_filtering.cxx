
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

// PValues for Sobel worker
OCPI::Util::PValue out_pvlist[] = {
	OCPI::Util::PVULong("bufferCount", 1),
	OCPI::Util::PVString("xferRole", "active"),
	OCPI::Util::PVULong("bufferSize", 0x4000),
	OCPI::Util::PVEnd
};

int main ( int argc, char* argv [ ] )
{
	if(argc != 2) {
		std::cout << std::endl
			<< "Usage: ./canny_edge_detector <image name>\n"
			<< std::endl;
		return 0;
	}

	try
	{
		( void ) argc;
		( void ) argv;

		/* ---- Load the image from file (grayscale) -------------- */
		IplImage* img = cvLoadImage( argv[1], 0 );
		cvNamedWindow( "Input", CV_WINDOW_AUTOSIZE );
		cvShowImage( "Input", img );
		/*
		cvWaitKey(0);
		return 0;
		*/
		// TODO - convert to 8U pixel depth (default image already is)

		// Create an image for the output (grayscale)
		IplImage* outImg = cvLoadImage( argv[1], 0 );
		cvNamedWindow( "Output", CV_WINDOW_AUTOSIZE );
		// TODO - create this image from scratch

		// Run Canny edge detection
		/*
		cvCanny( outImg, outImg, 10, 100, 3 );
		cvShowImage( "Output", outImg );
		*/

		/* ---- Create the shared RCC container and application -------------- */
		OCPI::Container::Interface* rcc_container ( Demo::get_rcc_interface ( ) );

		// Holds RCC worker interfaces passed to the RCC dispatch thread
		std::vector<OCPI::Container::Interface*> interfaces;
		interfaces.push_back ( rcc_container );

		OCPI::Container::Application*
			rcc_application ( rcc_container->createApplication ( ) );

		// Holds facades for group operations
		std::vector<Demo::WorkerFacade*> facades;

		/* ---- Create the sobel worker --------------------------------- */
		Demo::WorkerFacade xSobel ( "Sobel (RCC)",
				rcc_application,
				Demo::get_rcc_uri ( "sobel" ).c_str ( ),
				"sobel" );
		Demo::WorkerFacade ySobel ( "Sobel (RCC)",
				rcc_application,
				Demo::get_rcc_uri ( "sobel" ).c_str ( ),
				"sobel" );

		// Set properties
		OCPI::Util::PValue xWorker_pvlist[] = {
			OCPI::Util::PVULong("height", img->height),
			OCPI::Util::PVULong("width", img->width),
			OCPI::Util::PVULong("xderiv", 1),
			OCPI::Util::PVEnd
		};
		xSobel.set_properties(xWorker_pvlist);
		facades.push_back ( &xSobel );

		OCPI::Util::PValue yWorker_pvlist[] = {
			OCPI::Util::PVULong("height", img->height),
			OCPI::Util::PVULong("width", img->width),
			OCPI::Util::PVULong("xderiv", 0),
			OCPI::Util::PVEnd
		};
		ySobel.set_properties(yWorker_pvlist);
		// facades.push_back ( &ySobel );

		// Set ports
		OCPI::Container::Port
			&wOut = xSobel.port("out"),
			&wIn = xSobel.port("in");

		// Pass input to xSobel first
		// Output of xSobel feeds into ySobel
		// Output of ySobel goes to application
		// TODO - corner cases (doesn't quite work)
		// wOut.connect( ySobel.port("in") );
		// wIn.connect( xSobel.port("out") );

		// Set external ports (need 3 buffers for out)
		OCPI::Container::ExternalPort
			&myOut = wIn.connectExternal("aci_out", out_pvlist),
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

		// Current line
		uint8_t *imgLine = (uint8_t *) img->imageData;
		uint8_t *outImgLine = (uint8_t *) outImg->imageData;

		for(int i = 0; i < img->height; i++) {
			// std::cout << "Starting line..." << std::endl;

			// Set output data
			OCPI::Container::ExternalBuffer* myOutput = myOut.getBuffer(odata, olength);
			// printf("odata: %x\n", odata);
			memcpy(odata, imgLine, img->widthStep);
			imgLine += img->widthStep;
			myOutput->put(0, img->widthStep, false);

			std::cout << "My output buffer is size " << olength << std::endl;

			// Call dispatch so the worker can "act" on its input data
			std::for_each ( interfaces.begin(),
					interfaces.end(),
					Demo::dispatch );

			// std::cout << "Done working..." << std::endl;

			// Don't get result on first line
			if(i == 0)
				continue;

			// Get input data
			OCPI::Container::ExternalBuffer* myInput = myIn.getBuffer(opcode, idata,
				ilength, isEndOfData);

			std::cout << "My input buffer is size " << ilength << std::endl;
			std::cout << "The contents of the input buffer is ";
			if(idata == NULL) {
				std::cout << "[NULL]";
			}
			else {
				// Sanity check
				for(int j = 0; j < 10; j++)
					std::cout << (int) idata[j] << " ";
			}
			std::cout << std::endl;

			myInput->release();
			memcpy(outImgLine, idata, img->widthStep);
			outImgLine += img->widthStep;
		}
		// TODO - last line?

		// Show image
		cvShowImage( "Output", outImg );

		std::cout << "\nOpenOCPI application is done\n" << std::endl;

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


