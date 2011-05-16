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

static const double pi = 3.14159265358979323846;

int main ( int argc, char* argv [ ] )
{
  if(argc != 3) {
    printf("Usage: ./motion <imgA> <imgB>\n");
    return 0;
  }

  // load images
  IplImage* imgA_color = cvLoadImage( argv[1] );
  IplImage* imgB_color = cvLoadImage( argv[2] );
  IplImage* imgC = cvLoadImage( argv[1], 0 ); // for marking flow

  CvSize img_sz = cvGetSize( imgA_color );

  IplImage* imgA = cvCreateImage( img_sz, IPL_DEPTH_8U, 1 );
  IplImage* imgB = cvCreateImage( img_sz, IPL_DEPTH_8U, 1 );

  cvConvertImage( imgA_color, imgA );
  cvConvertImage( imgB_color, imgB );

	try
	{
		( void ) argc;
		( void ) argv;

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
			OCPI::Util::PVULong("height", imgA->height),
			OCPI::Util::PVULong("width", imgA->width),
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
			OCPI::Util::PVULong("height", imgA->height),
			OCPI::Util::PVULong("width", imgA->width),
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
			OCPI::Util::PVULong("height", imgA->height),
			OCPI::Util::PVULong("width", imgA->width),
			OCPI::Util::PVEnd
		};
		corner_worker.set_properties( corner_worker_pvlist );
		facades.push_back ( &corner_worker );

		OCPI::Container::Port
			&cornerOut = corner_worker.port("out"),
			&cornerIn = corner_worker.port("in");

    // good_features_to_track (copy)
		Demo::WorkerFacade features_workerA (
        "GoodFeaturesToTrackA (RCC)",
				rcc_application,
				Demo::get_rcc_uri ( "good_features_to_track" ).c_str ( ),
				"good_features_to_track" );

		features_workerA.set_properties( features_worker_pvlist );
		facades.push_back ( &features_workerA );

		OCPI::Container::Port
			&featuresAOut = features_workerA.port("out"),
			&featuresAIn = features_workerA.port("in");

    // min_eigen_val (copy)
		Demo::WorkerFacade min_workerA (
        "MinEigenValA (RCC)",
				rcc_application,
				Demo::get_rcc_uri ( "min_eigen_val" ).c_str ( ),
				"min_eigen_val" );

		min_workerA.set_properties( min_worker_pvlist );
		facades.push_back ( &min_workerA );

		OCPI::Container::Port
			&minAOut = min_workerA.port("out"),
			&minAIn = min_workerA.port("in");

    // corner_eigen_vals_vecs (copy)
		Demo::WorkerFacade corner_workerA (
        "CornerEigenValsVecsA (RCC)",
				rcc_application,
				Demo::get_rcc_uri ( "corner_eigen_vals_vecs" ).c_str ( ),
				"corner_eigen_vals_vecs" );

		corner_workerA.set_properties( corner_worker_pvlist );
		facades.push_back ( &corner_workerA );

		OCPI::Container::Port
			&cornerAOut = corner_workerA.port("out"),
			&cornerAIn = corner_workerA.port("in");

    // sobel_32f (A_dx)
		Demo::WorkerFacade sobel_adx_worker (
        "Sobel 32f A_dx (RCC)",
				rcc_application,
				Demo::get_rcc_uri ( "sobel_32f" ).c_str ( ),
				"sobel_32f" );

		OCPI::Util::PValue sobel_dx_worker_pvlist[] = {
			OCPI::Util::PVULong("height", imgA->height),
			OCPI::Util::PVULong("width", imgA->width),
			OCPI::Util::PVBool("xderiv", 1),
			OCPI::Util::PVEnd
		};

		sobel_adx_worker.set_properties( sobel_dx_worker_pvlist );
		facades.push_back ( &sobel_adx_worker );

		OCPI::Container::Port
			&sobelAdxOut = sobel_adx_worker.port("out_32f"),
			&sobelAdx8UOut = sobel_adx_worker.port("out"),
			&sobelAdxIn = sobel_adx_worker.port("in");

    // sobel_32f (A_dy)
		Demo::WorkerFacade sobel_ady_worker (
        "Sobel 32f A_dy (RCC)",
				rcc_application,
				Demo::get_rcc_uri ( "sobel_32f" ).c_str ( ),
				"sobel_32f" );

		OCPI::Util::PValue sobel_dy_worker_pvlist[] = {
			OCPI::Util::PVULong("height", imgA->height),
			OCPI::Util::PVULong("width", imgA->width),
			OCPI::Util::PVBool("xderiv", 0),
			OCPI::Util::PVEnd
		};

		sobel_ady_worker.set_properties( sobel_dy_worker_pvlist );
		facades.push_back ( &sobel_ady_worker );

		OCPI::Container::Port
			&sobelAdyOut = sobel_ady_worker.port("out_32f"),
			&sobelAdy8UOut = sobel_ady_worker.port("out"),
			&sobelAdyIn = sobel_ady_worker.port("in");
/*

    // sobel_32f (A_d2x)
		Demo::WorkerFacade sobel_ad2x_worker (
        "Sobel 32f A_d2x (RCC)",
				rcc_application,
				Demo::get_rcc_uri ( "sobel_32f" ).c_str ( ),
				"sobel_32f" );

		sobel_ad2x_worker.set_properties( sobel_dx_worker_pvlist );
		//facades.push_back ( &sobel_ad2x_worker );

		OCPI::Container::Port
			&sobelAd2xOut = sobel_ad2x_worker.port("out_32f"),
			&sobelAd2xIn = sobel_ad2x_worker.port("in");

    // sobel_32f (A_d2y)
		Demo::WorkerFacade sobel_ad2y_worker (
        "Sobel 32f A_d2y (RCC)",
				rcc_application,
				Demo::get_rcc_uri ( "sobel_32f" ).c_str ( ),
				"sobel_32f" );

		sobel_ad2y_worker.set_properties( sobel_dy_worker_pvlist );
		//facades.push_back ( &sobel_ad2y_worker );

		OCPI::Container::Port
			&sobelAd2yOut = sobel_ad2y_worker.port("out_32f"),
			&sobelAd2yIn = sobel_ad2y_worker.port("in");

    // sobel_32f (A_dxdy)
		Demo::WorkerFacade sobel_adxdy_worker (
        "Sobel 32f A_dxdy (RCC)",
				rcc_application,
				Demo::get_rcc_uri ( "sobel_32f" ).c_str ( ),
				"sobel_32f" );

		sobel_adxdy_worker.set_properties( sobel_dy_worker_pvlist );
		//facades.push_back ( &sobel_adxdy_worker );

		OCPI::Container::Port
			&sobelAdxdyOut = sobel_adxdy_worker.port("out_32f"),
			&sobelAdxdyIn = sobel_adxdy_worker.port("in");
*/

    // sobel_32f (B_dx)
		Demo::WorkerFacade sobel_bdx_worker (
        "Sobel 32f B_dx (RCC)",
				rcc_application,
				Demo::get_rcc_uri ( "sobel_32f" ).c_str ( ),
				"sobel_32f" );

		sobel_bdx_worker.set_properties( sobel_dx_worker_pvlist );
		facades.push_back ( &sobel_bdx_worker );

		OCPI::Container::Port
			&sobelBdxOut = sobel_bdx_worker.port("out_32f"),
			&sobelBdx8UOut = sobel_bdx_worker.port("out"),
			&sobelBdxIn = sobel_bdx_worker.port("in");

    // sobel_32f (B_dy)
		Demo::WorkerFacade sobel_bdy_worker (
        "Sobel 32f B_dy (RCC)",
				rcc_application,
				Demo::get_rcc_uri ( "sobel_32f" ).c_str ( ),
				"sobel_32f" );

		sobel_bdy_worker.set_properties( sobel_dy_worker_pvlist );
		facades.push_back ( &sobel_bdy_worker );

		OCPI::Container::Port
			&sobelBdyOut = sobel_bdy_worker.port("out_32f"),
			&sobelBdy8UOut = sobel_bdy_worker.port("out"),
			&sobelBdyIn = sobel_bdy_worker.port("in");

/*
    // optical_flow_pyr_lk
		Demo::WorkerFacade optical_flow_worker (
        "Optical Flow Pyr LK (RCC)",
				rcc_application,
				Demo::get_rcc_uri ( "optical_flow_pyr_lk" ).c_str ( ),
				"optical_flow_pyr_lk" );

		OCPI::Util::PValue optical_flow_worker_pvlist[] = {
			OCPI::Util::PVULong("height", imgA->height),
			OCPI::Util::PVULong("width", imgA->width),
			OCPI::Util::PVULong("win_height", 10),
			OCPI::Util::PVULong("win_width", 10),
			OCPI::Util::PVULong("level", 0),
			OCPI::Util::PVULong("term_max_count", 20),
			OCPI::Util::PVDouble("term_epsilon", 0.01),
			OCPI::Util::PVDouble("deriv_lambda", 0.5),
			OCPI::Util::PVEnd
		};
		optical_flow_worker.set_properties( optical_flow_worker_pvlist );
		//facades.push_back ( &optical_flow_worker );

		OCPI::Container::Port
			&opticalFlowInA = optical_flow_worker.port("in_A"),
			&opticalFlowInAdx = optical_flow_worker.port("in_Adx"),
			&opticalFlowInAdy = optical_flow_worker.port("in_Ady"),
			&opticalFlowInAd2x = optical_flow_worker.port("in_Ad2x"),
			&opticalFlowInAd2y = optical_flow_worker.port("in_Ad2y"),
			&opticalFlowInAdxdy = optical_flow_worker.port("in_Adxdy"),
			&opticalFlowInB = optical_flow_worker.port("in_B"),
			&opticalFlowInBdx = optical_flow_worker.port("in_Bdx"),
			&opticalFlowInBdy = optical_flow_worker.port("in_Bdy"),
			&opticalFlowInFeature = optical_flow_worker.port("in_feature"),
			&opticalFlowOut = optical_flow_worker.port("out"),
			&opticalFlowStatusOut = optical_flow_worker.port("out_status"),
			&opticalFlowErrOut = optical_flow_worker.port("out_err");
*/

    printf(">>> DONE INIT!\n");

		/* ---- Create connections --------------------------------- */

    minIn.connect( cornerOut );
    featuresIn.connect( minOut );
    //opticalFlowInFeature.connect( featuresOut );

    minAIn.connect( cornerAOut );
    featuresAIn.connect( minAOut );

    printf(">>> DONE CONNECTING (feature)!\n");

    /*
    opticalFlowInAdx.connect( sobelAdxOut );
    opticalFlowInAdy.connect( sobelAdyOut );
    opticalFlowInAd2x.connect( sobelAd2xOut );
    opticalFlowInAd2y.connect( sobelAd2yOut );
    opticalFlowInAdxdy.connect( sobelAdxdyOut );
    */

    printf(">>> DONE CONNECTING (A)!\n");

    /*
    opticalFlowInBdx.connect( sobelBdxOut );
    opticalFlowInBdy.connect( sobelBdyOut );
    */

    printf(">>> DONE CONNECTING (B)!\n");

    /*
    sobelAd2xIn.connect( sobelAdx8UOut );
    sobelAd2yIn.connect( sobelAdy8UOut );
    sobelAdxdyIn.connect( sobelAdx8UOut );
    */

    printf(">>> DONE CONNECTING (Sobel)!\n");

		// Set external ports
    /*
		OCPI::Container::ExternalPort
			&myOutFeature = cornerIn.connectExternal("aci_out"),
			&myOutFeatureA = cornerAIn.connectExternal("aci_outA"),
			&myOutA = opticalFlowInA.connectExternal("aci_out_A"),
			&myOutAdx = sobelAdxIn.connectExternal("aci_out_Adx"),
			&myOutAdy = sobelAdyIn.connectExternal("aci_out_Ady"),
			&myOutB = opticalFlowInB.connectExternal("aci_out_B"),
			&myOutBdx = sobelBdxIn.connectExternal("aci_out_Bdx"),
			&myOutBdy = sobelBdyIn.connectExternal("aci_out_Bdy"),
			&myIn = opticalFlowOut.connectExternal("aci_in"),
			&myInStatus = opticalFlowStatusOut.connectExternal("aci_in_status"),
			&myInErr = opticalFlowErrOut.connectExternal("aci_in_err"),
      &myInFeature = featuresAOut.connectExternal("aci_in_feature");
    */
		OCPI::Container::ExternalPort
			&myOutAdx = sobelAdxIn.connectExternal("aci_out_Adx"),
			&myOutAdy = sobelAdyIn.connectExternal("aci_out_Ady"),
      &myInAdx = sobelAdx8UOut.connectExternal("aci_in_Adx"),
      &myInAdy = sobelAdy8UOut.connectExternal("aci_in_Ady"),
      &myIn32fAdx = sobelAdxOut.connectExternal("aci_in_32f_Adx"),
      &myIn32fAdy = sobelAdyOut.connectExternal("aci_in_32f_Ady");

		OCPI::Container::ExternalPort
			&myOutFeature = cornerIn.connectExternal("aci_out"),
			&myOutFeatureA = cornerAIn.connectExternal("aci_outA"),
      &myInFeature = featuresOut.connectExternal("aci_in_feature"),
      &myInFeatureA = featuresAOut.connectExternal("aci_in_featureA");

		OCPI::Container::ExternalPort
			&myOutBdx = sobelBdxIn.connectExternal("aci_out_Bdx"),
			&myOutBdy = sobelBdyIn.connectExternal("aci_out_Bdy"),
      &myInBdx = sobelBdx8UOut.connectExternal("aci_in_Bdx"),
      &myInBdy = sobelBdy8UOut.connectExternal("aci_in_Bdy"),
      &myIn32fBdx = sobelBdxOut.connectExternal("aci_in_32f_Bdx"),
      &myIn32fBdy = sobelBdyOut.connectExternal("aci_in_32f_Bdy");

    printf(">>> DONE CONNECTING (all)!\n");

		/* ---- Start all of the workers ------------------------------------- */
		std::for_each ( facades.rbegin ( ), facades.rend ( ), Demo::start );

		// Note: We must call dispatch before the first call to 
		// ExternalPort::getBuffer or else it will seg fault
		std::for_each ( interfaces.begin(), interfaces.end(), Demo::dispatch );

    printf(">>> DONE STARTING!\n");
    printf(">>> WORKERS: %d\n", facades.size());

		// Output info
		uint8_t *odata;
		uint32_t olength;

		// Input info
		uint8_t *idata;
		uint32_t ilength;
		uint8_t opcode = 0;
		bool isEndOfData = false;

    // Set output data
    OCPI::Container::ExternalBuffer* myOutput;
    
    myOutput = myOutFeature.getBuffer(odata, olength);
    memcpy(odata, imgA->imageData, imgA->height * imgA->width);
    myOutput->put(0, imgA->height * imgA->width, false);

    myOutput = myOutFeatureA.getBuffer(odata, olength);
    memcpy(odata, imgA->imageData, imgA->height * imgA->width);
    myOutput->put(0, imgA->height * imgA->width, false);

    /*
    myOutput = myOutA.getBuffer(odata, olength);
    memcpy(odata, imgA->imageData, imgA->height * imgA->width);
    myOutput->put(0, imgA->height * imgA->width, false);
    */

    myOutput = myOutAdx.getBuffer(odata, olength);
    memcpy(odata, imgA->imageData, imgA->height * imgA->width);
    myOutput->put(0, imgA->height * imgA->width, false);

    myOutput = myOutAdy.getBuffer(odata, olength);
    memcpy(odata, imgA->imageData, imgA->height * imgA->width);
    myOutput->put(0, imgA->height * imgA->width, false);

    /*
    myOutput = myOutB.getBuffer(odata, olength);
    memcpy(odata, imgB->imageData, imgB->height * imgB->width);
    myOutput->put(0, imgB->height * imgB->width, false);
    */

    myOutput = myOutBdx.getBuffer(odata, olength);
    memcpy(odata, imgB->imageData, imgB->height * imgB->width);
    myOutput->put(0, imgB->height * imgB->width, false);

    myOutput = myOutBdy.getBuffer(odata, olength);
    memcpy(odata, imgB->imageData, imgB->height * imgB->width);
    myOutput->put(0, imgB->height * imgB->width, false);

    std::cout << "My output buffer is size " << olength << std::endl;

    // Call dispatch so the worker can "act" on its input data
    for( int i = 0; i < 3; i++ ) {
      printf(">>> DISPATCHING: %d\n", i);
      std::for_each ( interfaces.begin(), interfaces.end(), Demo::dispatch );
    }

    // Get input data
    OCPI::Container::ExternalBuffer* myInput;

    /*
    myInput = myInFeature.getBuffer(opcode, idata, ilength, isEndOfData);
    size_t ncorners = ilength / (2 * sizeof(float));
    float *cornersA = (float *) malloc(ncorners * 2 * sizeof(float));
    memcpy(cornersA, idata, ilength);
    myInput->release();
    std::cout << "My old corners " << ncorners << std::endl;
    */

    /*
    for( size_t i = 0; i < ncorners; i++) {
      float x = cornersA[2*i], y = cornersA[2*i+1];
      cvCircle( imgC, cvPoint( cvRound(x), cvRound(y) ), 5, CV_RGB(255, 0, 0), 2 );
    }
    */

    // Test gradients
    myInput = myInAdx.getBuffer(opcode, idata, ilength, isEndOfData);
    std::cout << "My size: " << ilength << ", H*W: "
              << imgA->height*imgA->width << " "
              << imgB->height*imgB->width << std::endl;
    // myInput->release();
    memcpy(imgC->imageData, idata, ilength);

    // Get corners, statuses, errors
    /*
    myInput = myIn.getBuffer(opcode, idata, ilength, isEndOfData);
    // size_t ncorners = ilength / (2 * sizeof(float));
    float *cornersB = (float *) malloc(ncorners * 2 * sizeof(float));
    memcpy(cornersB, idata, ilength);
    // myInput->release();
    std::cout << "My corners " << ilength / (2 * sizeof(float)) << std::endl;

    myInput = myInStatus.getBuffer(opcode, idata, ilength, isEndOfData);
    char *status = (char *) malloc(ncorners * sizeof(char));
    memcpy(status, idata, ilength);
    // myInput->release();

    myInput = myInErr.getBuffer(opcode, idata, ilength, isEndOfData);
    float *err = (float *) malloc(ncorners * sizeof(float));
    memcpy(err, idata, ilength);
    // myInput->release();

    // Draw flow
    for( size_t i = 0; i < ncorners; i++ ) {
      if( status[i] == 0 || err[i] > 500 )
        continue;

      double x0 = cornersA[2*i];
      double y0 = cornersA[2*i+1];
      CvPoint p = cvPoint( cvRound(x0), cvRound(y0) );
      double x1 = cornersB[2*i];
      double y1 = cornersB[2*i+1];
      CvPoint q = cvPoint( cvRound(x1), cvRound(y1) );

      CvScalar line_color = CV_RGB(255, 0, 0);
      int line_thickness = 1;

      // Main line (p -> q) lengthened
      double angle = atan2( (double) y1 - y0, (double) x1 - x0 );
      double hypotenuse = 1.5;
      q.x = cvRound(x0 + 6 * hypotenuse * cos(angle));
      q.y = cvRound(y0 + 6 * hypotenuse * sin(angle));
      cvLine( imgC, p, q, line_color, line_thickness, CV_AA, 0 );

      // Arrows
      p.x = (int) (x0 + 5 * hypotenuse * cos(angle + pi / 4));
      p.y = (int) (y0 + 5 * hypotenuse * sin(angle + pi / 4));
      cvLine( imgC, p, q, line_color, line_thickness, CV_AA, 0 );

      p.x = (int) (x0 + 5 * hypotenuse * cos(angle - pi / 4));
      p.y = (int) (y0 + 5 * hypotenuse * sin(angle - pi / 4));
      cvLine( imgC, p, q, line_color, line_thickness, CV_AA, 0 );
    }
    */

    // Save image
    cvSaveImage("output_image.jpg", imgC);

		std::cout << "\nOpenOCPI application is done\n" << std::endl;

    // display
    cvNamedWindow( "Image A", CV_WINDOW_AUTOSIZE );
    cvNamedWindow( "Image B", CV_WINDOW_AUTOSIZE );
    cvNamedWindow( "Image Flow", CV_WINDOW_AUTOSIZE );

    cvShowImage( "Image A", imgA );
    cvShowImage( "Image B", imgB );
    cvShowImage( "Image Flow", imgC );

    // cleanup
    cvWaitKey(0);

    cvReleaseImage( &imgA );
    cvReleaseImage( &imgB );
    cvReleaseImage( &imgC );

    cvDestroyWindow( "Image A" );
    cvDestroyWindow( "Image B" );
    cvDestroyWindow( "Image Flow" );

    /*
    free( cornersA );
    free( cornersB );
    free( status );
    free( err );
    */
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


