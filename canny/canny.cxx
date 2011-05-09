
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
			<< "Usage: ./canny <image_name>\n"
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

		/* ---- Create the workers --------------------------------- */
    // canny_partial
		Demo::WorkerFacade canny_worker (
        "Canny (RCC)",
				rcc_application,
				Demo::get_rcc_uri ( "canny_partial" ).c_str ( ),
				"canny_partial" );

		OCPI::Util::PValue canny_worker_pvlist[] = {
			OCPI::Util::PVULong("height", img->height),
			OCPI::Util::PVULong("width", img->width),
      OCPI::Util::PVDouble("low_thresh", 10), // Canny
      OCPI::Util::PVDouble("high_thresh", 100), // Canny
			OCPI::Util::PVEnd
		};
		canny_worker.set_properties( canny_worker_pvlist );
		facades.push_back ( &canny_worker );

		OCPI::Container::Port
			&cOut = canny_worker.port("out"),
			&cIn_dx = canny_worker.port("in_dx"),
			&cIn_dy = canny_worker.port("in_dy");

    // sobel_x
		Demo::WorkerFacade sobel_x_worker (
        "Sobel X (RCC)",
				rcc_application,
				Demo::get_rcc_uri ( "sobel" ).c_str ( ),
				"sobel" );

		OCPI::Util::PValue sobel_x_worker_pvlist[] = {
			OCPI::Util::PVULong("height", img->height),
			OCPI::Util::PVULong("width", img->width),
      OCPI::Util::PVULong("xderiv", 1), // TODO change to bool
			OCPI::Util::PVEnd
		};
		sobel_x_worker.set_properties( sobel_x_worker_pvlist );
		facades.push_back ( &sobel_x_worker );

		OCPI::Container::Port
			&sxOut = sobel_x_worker.port("out"),
			&sxIn = sobel_x_worker.port("in");

    // sobel_y
		Demo::WorkerFacade sobel_y_worker (
        "Sobel Y (RCC)",
				rcc_application,
				Demo::get_rcc_uri ( "sobel" ).c_str ( ),
				"sobel" );

		OCPI::Util::PValue sobel_y_worker_pvlist[] = {
			OCPI::Util::PVULong("height", img->height),
			OCPI::Util::PVULong("width", img->width),
      OCPI::Util::PVULong("xderiv", 0), // TODO change to bool
			OCPI::Util::PVEnd
		};
		sobel_y_worker.set_properties( sobel_y_worker_pvlist );
		facades.push_back ( &sobel_y_worker );

		OCPI::Container::Port
			&syOut = sobel_y_worker.port("out"),
			&syIn = sobel_y_worker.port("in");

		/* ---- Create connections --------------------------------- */

    cIn_dx.connect( sxOut );
    cIn_dy.connect( syOut );

		// Set external ports
		OCPI::Container::ExternalPort
			&myOutX = sxIn.connectExternal("aci_out_dx"),
			&myOutY = syIn.connectExternal("aci_out_dy"),
			&myIn = cOut.connectExternal("aci_in");

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

		// Current line
		uint8_t *imgLine = (uint8_t *) img->imageData;

		for(int i = 0; i < img->height; i++) {
      printf(">>> ACI sending line: %d\n", i);
			// Set output data
			OCPI::Container::ExternalBuffer* myOutput;
      
      myOutput = myOutX.getBuffer(odata, olength);
			memcpy(odata, imgLine, img->widthStep);
			myOutput->put(0, img->widthStep, false);

      printf(">>> ACI got X\n");

      myOutput = myOutY.getBuffer(odata, olength);
			memcpy(odata, imgLine, img->widthStep);
			myOutput->put(0, img->widthStep, false);

      printf(">>> ACI got Y\n");

			imgLine += img->widthStep;

			std::cout << "My output buffer is size " << olength << std::endl;

			// Call dispatch so the worker can "act" on its input data
			std::for_each ( interfaces.begin(),
					interfaces.end(),
					Demo::dispatch );
		}
		// TODO - last line?
    std::for_each ( interfaces.begin(),
        interfaces.end(),
        Demo::dispatch );
    std::for_each ( interfaces.begin(), // TODO - twice!
        interfaces.end(),
        Demo::dispatch );

    // Get input data
    OCPI::Container::ExternalBuffer* myInput = myIn.getBuffer(opcode, idata,
      ilength, isEndOfData);

    std::cout << "My input buffer is size " << ilength << std::endl;

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


