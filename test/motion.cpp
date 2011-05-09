
// Optical Flow Demo App
// Tony Liu (tonyliu@mit.edu)
// 

#include "highgui.h"
#include "cv.h"
#include "cxcore.h"

#include <cstdio>
#include <cmath>

#define USE_OPENCV
#define OPTICAL_FLOW
#define SHOW_GRAYSCALE

static const double pi = 3.14159265358979323846;

// ----- Optical Flow via Lucas-Kanade

#ifdef OPTICAL_FLOW
const int MAX_CORNERS = 100;
const int win_size = 10;

// Temporary buffers
IplImage* eig_image = NULL;
IplImage* tmp_image = NULL;
IplImage* pyrA = NULL;
IplImage* pyrB = NULL;

// Storage for corners and features
CvPoint2D32f cornersA[ MAX_CORNERS ];
CvPoint2D32f cornersB[ MAX_CORNERS ];
char features_found[ MAX_CORNERS ];
float feature_errors[ MAX_CORNERS ];

// Parameters
// - imgA and imgB: frames in sequence (8U single channel)
// - imgC: original frame to mark (RGB is fine)
void calcOpticalFlowAndMark(IplImage *imgA, IplImage *imgB, IplImage *imgC) {
  // Create buffers if necessary
  CvSize img_sz = cvGetSize( imgA );
  if( !eig_image )
    eig_image = cvCreateImage( img_sz, IPL_DEPTH_32F, 1 );
  if( !tmp_image )
    tmp_image = cvCreateImage( img_sz, IPL_DEPTH_32F, 1 );

  // Find features to track
  int corner_count = MAX_CORNERS;
  cvGoodFeaturesToTrack(
    imgA,
    eig_image,
    tmp_image,
    cornersA,
    &corner_count,
    0.03, // quality_level
    5.0, // min_distance
    NULL,
    3, // block_size (default)
    0, // use_harris (default)
    0.04 // k (default)
  );
  cvFindCornerSubPix(
    imgA,
    cornersA,
    corner_count,
    cvSize(win_size, win_size),
    cvSize(-1, -1),
    cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS, 20, 0.03)
  );

  // Call the Lucas-Kanade algorithm
  int flags = CV_LKFLOW_PYR_A_READY;
  CvSize pyr_sz = cvSize( imgA->width+8, imgB->height/3 );
  if( !pyrA || !pyrB ) {
    pyrA = cvCreateImage( pyr_sz, IPL_DEPTH_32F, 1 );
    pyrB = cvCreateImage( pyr_sz, IPL_DEPTH_32F, 1 );
    flags = 0;
  }

  cvCalcOpticalFlowPyrLK(
    imgA,
    imgB,
    pyrA,
    pyrB,
    cornersA,
    cornersB,
    corner_count,
    cvSize( win_size, win_size ),
    5,
    features_found,
    feature_errors,
    cvTermCriteria( CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 20, .3 ),
    flags
  );

  // Draw resulting velocity vectors
  for( int i = 0; i < corner_count; i++ ) {
    if( features_found[i] == 0 || feature_errors[i] > 550 ) {
      // printf("Error is %f/n", feature_errors[i]);
      continue;
    }

    double x0 = cornersA[i].x;
    double y0 = cornersA[i].y;
    CvPoint p = cvPoint( cvRound(x0), cvRound(y0) );
    double x1 = cornersB[i].x;
    double y1 = cornersB[i].y;
    CvPoint q = cvPoint( cvRound(x1), cvRound(y1) );
    //if( sqrt( (double) (y1-y0)*(y1-y0) + (x1-x0)*(x1-x0) ) < 0.1 )
    //if(fabs(y1 - y0) < .5 || fabs(x1 - x0) < .5)
    //  continue;
    //printf("%.4lf %.4lf -> %.4lf %.4lf\n", x0, y0, x1, y1);

    CvScalar line_color = CV_RGB(255, 0, 0);
    int line_thickness = 1;
    //cvLine( imgC, p, q, CV_RGB(255,0,0), 2 );

    // Main line (p -> q) lengthened
    double angle = atan2( (double) y1 - y0, (double) x1 - x0 );
    double hypotenuse = sqrt( (double) (y1-y0)*(y1-y0) + (x1-x0)*(x1-x0) );
    if(hypotenuse < 1.01)
      hypotenuse = 1.01;
    if(hypotenuse > 1.99)
      hypotenuse = 1.99;
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
}
#endif

// ----- For OpenCV GUIs
int g_slider_position = 0;
CvCapture *g_capture = NULL;

void onTrackbarSlide(int pos) {
  cvSetCaptureProperty(
      g_capture,
      CV_CAP_PROP_POS_FRAMES,
      pos
      );
}

// ----- Main
int main( int argc, char** argv ) {
  if(argc != 2) {
    printf("Usage: ./motion <video name>\n");
    return 0;
  }

  cvNamedWindow( "Example Video", CV_WINDOW_AUTOSIZE );
  g_capture = cvCreateFileCapture( argv[1] );
  int frames = (int) cvGetCaptureProperty(
      g_capture,
      CV_CAP_PROP_FRAME_COUNT
      );
  if( frames != 0 ) {
    cvCreateTrackbar(
        "Position",
        "Example Video",
        &g_slider_position,
        frames,
        onTrackbarSlide
        );
  }

  // Keep track of frames
  IplImage *prev_frame;
  IplImage *cur_frame = cvQueryFrame( g_capture ); // read first frame
  CvSize img_sz = cvGetSize( cur_frame );

  IplImage* imgA = cvCreateImage( img_sz, IPL_DEPTH_8U, 1 );
  IplImage* imgB = cvCreateImage( img_sz, IPL_DEPTH_8U, 1 );
  cvConvertImage( cur_frame, imgB ); // convert first frame

  IplImage* imgC = cvCreateImage( img_sz, cur_frame->depth, cur_frame->nChannels );

  while(1) {
    // Scroll to next frame and read
#ifdef OPTICAL_FLOW
    if( pyrB )
      cvCopy( pyrB, pyrA );
    if( imgB )
      cvCopy( imgB, imgA );
    if( cur_frame )
      cvCopy( cur_frame, prev_frame );
#endif
    cur_frame = cvQueryFrame( g_capture );
    if( !cur_frame )
      break;

#ifdef OPTICAL_FLOW
    // Convert frames to 8U single channel
    cvConvertImage( cur_frame, imgB );
    cvCopyImage( cur_frame, imgC );
    calcOpticalFlowAndMark( imgA, imgB, imgC );
    cvShowImage( "Example Video", imgC );
#else
    cvShowImage( "Example Video", cur_frame );
#endif

    char c = cvWaitKey( 10 ); // ms to wait
    if( c == 27 ) // ESC key
      break;
  }
  cvReleaseCapture( &g_capture );
  cvDestroyWindow( "Example Video" );

  return 0;
}

