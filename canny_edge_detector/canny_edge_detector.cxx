
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
		printf("Usage: ./canny_edge_detector <image name>\n");
		return 0;
	}

	try
	{
		( void ) argc;
		( void ) argv;

		std::vector<Demo::WorkerFacade*> facades;
		// Holds facades for group operations

		std::vector<OCPI::Container::Interface*> interfaces;
		// Holds RCC worker interfaces passed to the RCC dispatch thread

		/* ---- Create the shared RCC container and application -------------- */

		OCPI::Container::Interface* rcc_container ( Demo::get_rcc_interface ( ) );

		interfaces.push_back ( rcc_container );

		OCPI::Container::Application*
			rcc_application ( rcc_container->createApplication ( ) );

		/* ---- Create the copy worker --------------------------------- */

		Demo::WorkerFacade copy ( "Copy (RCC)",
				rcc_application,
				Demo::get_rcc_uri ( "copy" ).c_str ( ),
				"copy" );

		facades.push_back ( &copy );

		OCPI::Container::ExternalPort &myOut = 
			copy.port("in").connectExternal("aci_out");
		OCPI::Container::ExternalPort &myIn = 
			copy.port("out").connectExternal("aci_in");

		/* ---- Start all of the workers ------------------------------------- */

		std::for_each ( facades.rbegin ( ),
				facades.rend ( ),
				Demo::start );

		// Note: We must call dispatch before the first call to 
		// ExternalPort::getBuffer or else it will seg fault
		std::for_each ( interfaces.begin(),
				interfaces.end(),
				Demo::dispatch );

		uint8_t *odata;
		uint32_t olength;

		OCPI::Container::ExternalBuffer* myOutput = myOut.getBuffer(odata,olength);
		std::cout << "My output buffer is size " << olength << std::endl;

		// ---> Adding the data!
		strcpy((char*)odata,"hello world");

		myOutput->put(0, strlen("hello world") + 1, false);

		// TODO - add image data
		// TODO - how to use a stream? buffer size of 2048 is too small for images
		IplImage* img = cvLoadImage( argv[1] );
		cvNamedWindow( "Input", CV_WINDOW_AUTOSIZE );
		cvShowImage( "Input", img );

		// Create an image for the output (grayscale)
		IplImage* out = cvLoadImage( argv[1], 0 );
		cvNamedWindow( "Output", CV_WINDOW_AUTOSIZE );

		// Run Canny edge detection
		cvCanny( out, out, 10, 100, 3 );
		cvShowImage( "Output", out );

		// ---> End

		// Call dispatch so the worker can "act" on its input data
		std::for_each ( interfaces.begin(),
				interfaces.end(),
				Demo::dispatch );

		uint8_t *idata;
		uint32_t ilength;
		uint8_t opcode = 0;
		bool isEndOfData = false;

		// ---> Get the data!

		// TODO - retrieve image data
		OCPI::Container::ExternalBuffer* myInput = myIn.getBuffer(opcode,idata,
				ilength,isEndOfData);
		std::cout << "My input buffer is size " << ilength << std::endl;

		std::cout << "The contents of the input buffer is " << (char*)idata << 
			std::endl;

		std::cout << "\nOpenOCPI application is done\n" << std::endl;

		// Cleanup
		cvWaitKey(0);
		cvReleaseImage( &img );
		cvReleaseImage( &out );
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


