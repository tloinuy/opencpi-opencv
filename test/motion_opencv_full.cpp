
// Optical Flow Demo App
// Tony Liu (tonyliu@mit.edu)
// 

#include "highgui.h"
#include "cv.h"
#include "cvinternal.h"
#include "cxcore.h"

#include <cstdio>
#include <cmath>
#include <vector>

#define USE_OPENCV
#define OPTICAL_FLOW
#define SHOW_GRAYSCALE

static const double pi = 3.14159265358979323846;
using namespace std;

// -- rewrite

namespace cv
{

void test_calcOpticalFlowPyrLK( const Mat& prevImg, const Mat& nextImg,
                           const vector<Point2f>& prevPts,
                           vector<Point2f>& nextPts,
                           vector<uchar>& status, vector<float>& err,
                           Size winSize, int maxLevel,
                           TermCriteria criteria,
                           double derivLambda = 0.5,
                           int flags = 0 )
{
    derivLambda = std::min(std::max(derivLambda, 0.), 1.);
    double lambda1 = 1. - derivLambda, lambda2 = derivLambda;
    const int derivKernelSize = 3;
    const float deriv1Scale = 0.5f/4.f;
    const float deriv2Scale = 0.25f/4.f;
    const int derivDepth = CV_32F;
    Point2f halfWin((winSize.width-1)*0.5f, (winSize.height-1)*0.5f);

    CV_Assert( maxLevel >= 0 && winSize.width > 2 && winSize.height > 2 );
    CV_Assert( prevImg.size() == nextImg.size() &&
        prevImg.type() == nextImg.type() );

    size_t npoints = prevPts.size();
    nextPts.resize(npoints);
    status.resize(npoints);
    for( size_t i = 0; i < npoints; i++ )
        status[i] = true;
    err.resize(npoints);

    if( npoints == 0 )
        return;
    
    vector<Mat> prevPyr, nextPyr;

    int cn = prevImg.channels();
    buildPyramid( prevImg, prevPyr, maxLevel );
    buildPyramid( nextImg, nextPyr, maxLevel );
    // I, dI/dx ~ Ix, dI/dy ~ Iy, d2I/dx2 ~ Ixx, d2I/dxdy ~ Ixy, d2I/dy2 ~ Iyy
    Mat derivIBuf((prevImg.rows + winSize.height*2),
                  (prevImg.cols + winSize.width*2),
                  CV_MAKETYPE(derivDepth, cn*6));
    // J, dJ/dx ~ Jx, dJ/dy ~ Jy
    Mat derivJBuf((prevImg.rows + winSize.height*2),
                  (prevImg.cols + winSize.width*2),
                  CV_MAKETYPE(derivDepth, cn*3));
    Mat tempDerivBuf(prevImg.size(), CV_MAKETYPE(derivIBuf.type(), cn));
    Mat derivIWinBuf(winSize, derivIBuf.type());

    if( (criteria.type & TermCriteria::COUNT) == 0 )
        criteria.maxCount = 30;
    else
        criteria.maxCount = std::min(std::max(criteria.maxCount, 0), 100);
    if( (criteria.type & TermCriteria::EPS) == 0 )
        criteria.epsilon = 0.01;
    else
        criteria.epsilon = std::min(std::max(criteria.epsilon, 0.), 10.);
    criteria.epsilon *= criteria.epsilon;

    for( int level = maxLevel; level >= 0; level-- )
    {
        int k;
        Size imgSize = prevPyr[level].size();
        Mat tempDeriv( imgSize, tempDerivBuf.type(), tempDerivBuf.data );
        Mat _derivI( imgSize.height + winSize.height*2,
            imgSize.width + winSize.width*2,
            derivIBuf.type(), derivIBuf.data );
        Mat _derivJ( imgSize.height + winSize.height*2,
            imgSize.width + winSize.width*2,
            derivJBuf.type(), derivJBuf.data );
        Mat derivI(_derivI, Rect(winSize.width, winSize.height, imgSize.width, imgSize.height));
        Mat derivJ(_derivJ, Rect(winSize.width, winSize.height, imgSize.width, imgSize.height));
        CvMat cvderivI = _derivI;
        cvZero(&cvderivI);
        CvMat cvderivJ = _derivJ;
        cvZero(&cvderivJ);

        vector<int> fromTo(cn*2);
        for( k = 0; k < cn; k++ )
            fromTo[k*2] = k;

        prevPyr[level].convertTo(tempDeriv, derivDepth);
        for( k = 0; k < cn; k++ )
            fromTo[k*2+1] = k*6;
        mixChannels(&tempDeriv, 1, &derivI, 1, &fromTo[0], cn);

        // compute spatial derivatives and merge them together
        Sobel(prevPyr[level], tempDeriv, derivDepth, 1, 0, derivKernelSize, deriv1Scale );
        for( k = 0; k < cn; k++ )
            fromTo[k*2+1] = k*6 + 1;
        mixChannels(&tempDeriv, 1, &derivI, 1, &fromTo[0], cn);

        Sobel(prevPyr[level], tempDeriv, derivDepth, 0, 1, derivKernelSize, deriv1Scale );
        for( k = 0; k < cn; k++ )
            fromTo[k*2+1] = k*6 + 2;
        mixChannels(&tempDeriv, 1, &derivI, 1, &fromTo[0], cn);

        Sobel(prevPyr[level], tempDeriv, derivDepth, 2, 0, derivKernelSize, deriv2Scale );
        for( k = 0; k < cn; k++ )
            fromTo[k*2+1] = k*6 + 3;
        mixChannels(&tempDeriv, 1, &derivI, 1, &fromTo[0], cn);

        Sobel(prevPyr[level], tempDeriv, derivDepth, 1, 1, derivKernelSize, deriv2Scale );
        for( k = 0; k < cn; k++ )
            fromTo[k*2+1] = k*6 + 4;
        mixChannels(&tempDeriv, 1, &derivI, 1, &fromTo[0], cn);

        Sobel(prevPyr[level], tempDeriv, derivDepth, 0, 2, derivKernelSize, deriv2Scale );
        for( k = 0; k < cn; k++ )
            fromTo[k*2+1] = k*6 + 5;
        mixChannels(&tempDeriv, 1, &derivI, 1, &fromTo[0], cn);

        nextPyr[level].convertTo(tempDeriv, derivDepth);
        for( k = 0; k < cn; k++ )
            fromTo[k*2+1] = k*3;
        mixChannels(&tempDeriv, 1, &derivJ, 1, &fromTo[0], cn);

        Sobel(nextPyr[level], tempDeriv, derivDepth, 1, 0, derivKernelSize, deriv1Scale );
        for( k = 0; k < cn; k++ )
            fromTo[k*2+1] = k*3 + 1;
        mixChannels(&tempDeriv, 1, &derivJ, 1, &fromTo[0], cn);

        Sobel(nextPyr[level], tempDeriv, derivDepth, 0, 1, derivKernelSize, deriv1Scale );
        for( k = 0; k < cn; k++ )
            fromTo[k*2+1] = k*3 + 2;
        mixChannels(&tempDeriv, 1, &derivJ, 1, &fromTo[0], cn);

        /*copyMakeBorder( derivI, _derivI, winSize.height, winSize.height,
            winSize.width, winSize.width, BORDER_CONSTANT );
        copyMakeBorder( derivJ, _derivJ, winSize.height, winSize.height,
            winSize.width, winSize.width, BORDER_CONSTANT );*/

        for( size_t ptidx = 0; ptidx < npoints; ptidx++ )
        {
            Point2f prevPt = prevPts[ptidx]*(float)(1./(1 << level));
            Point2f nextPt;
            if( level == maxLevel )
            {
                if( flags & OPTFLOW_USE_INITIAL_FLOW )
                    nextPt = nextPts[ptidx]*(float)(1./(1 << level));
                else
                    nextPt = prevPt;
            }
            else
                nextPt = nextPts[ptidx]*2.f;
            nextPts[ptidx] = nextPt;
            
            Point2i iprevPt, inextPt;
            prevPt -= halfWin;
            iprevPt.x = cvFloor(prevPt.x);
            iprevPt.y = cvFloor(prevPt.y);

            if( iprevPt.x < -winSize.width || iprevPt.x >= derivI.cols ||
                iprevPt.y < -winSize.height || iprevPt.y >= derivI.rows )
            {
                if( level == 0 )
                {
                    status[ptidx] = false;
                    err[ptidx] = FLT_MAX;
                }
                continue;
            }
            
            float a = prevPt.x - iprevPt.x;
            float b = prevPt.y - iprevPt.y;
            float w00 = (1.f - a)*(1.f - b), w01 = a*(1.f - b);
            float w10 = (1.f - a)*b, w11 = a*b;
            size_t stepI = derivI.step/derivI.elemSize1();
            size_t stepJ = derivJ.step/derivJ.elemSize1();
            int cnI = cn*6, cnJ = cn*3;
            double A11 = 0, A12 = 0, A22 = 0;
            double iA11 = 0, iA12 = 0, iA22 = 0;
            
            // extract the patch from the first image
            int x, y;
            for( y = 0; y < winSize.height; y++ )
            {
                const float* src = (const float*)(derivI.data +
                    (y + iprevPt.y)*derivI.step) + iprevPt.x*cnI;
                float* dst = (float*)(derivIWinBuf.data + y*derivIWinBuf.step);

                for( x = 0; x < winSize.width*cnI; x += cnI, src += cnI )
                {
                    float I = src[0]*w00 + src[cnI]*w01 + src[stepI]*w10 + src[stepI+cnI]*w11;
                    dst[x] = I;
                    
                    float Ix = src[1]*w00 + src[cnI+1]*w01 + src[stepI+1]*w10 + src[stepI+cnI+1]*w11;
                    float Iy = src[2]*w00 + src[cnI+2]*w01 + src[stepI+2]*w10 + src[stepI+cnI+2]*w11;
                    dst[x+1] = Ix; dst[x+2] = Iy;
                    
                    float Ixx = src[3]*w00 + src[cnI+3]*w01 + src[stepI+3]*w10 + src[stepI+cnI+3]*w11;
                    float Ixy = src[4]*w00 + src[cnI+4]*w01 + src[stepI+4]*w10 + src[stepI+cnI+4]*w11;
                    float Iyy = src[5]*w00 + src[cnI+5]*w01 + src[stepI+5]*w10 + src[stepI+cnI+5]*w11;
                    dst[x+3] = Ixx; dst[x+4] = Ixy; dst[x+5] = Iyy;

                    iA11 += (double)Ix*Ix;
                    iA12 += (double)Ix*Iy;
                    iA22 += (double)Iy*Iy;

                    A11 += (double)Ixx*Ixx + (double)Ixy*Ixy;
                    A12 += Ixy*((double)Ixx + Iyy);
                    A22 += (double)Ixy*Ixy + (double)Iyy*Iyy;
                }
            }

            A11 = lambda1*iA11 + lambda2*A11;
            A12 = lambda1*iA12 + lambda2*A12;
            A22 = lambda1*iA22 + lambda2*A22;

            double D = A11*A22 - A12*A12;
            double minEig = (A22 + A11 - std::sqrt((A11-A22)*(A11-A22) +
                4.*A12*A12))/(2*winSize.width*winSize.height);
            err[ptidx] = (float)minEig;

            if( D < DBL_EPSILON )
            {
                if( level == 0 )
                    status[ptidx] = false;
                continue;
            }
            
            D = 1./D;

            nextPt -= halfWin;
            Point2f prevDelta;

            for( int j = 0; j < criteria.maxCount; j++ )
            {
                inextPt.x = cvFloor(nextPt.x);
                inextPt.y = cvFloor(nextPt.y);

                if( inextPt.x < -winSize.width || inextPt.x >= derivJ.cols ||
                    inextPt.y < -winSize.height || inextPt.y >= derivJ.rows )
                {
                    if( level == 0 )
                        status[ptidx] = false;
                    break;
                }

                a = nextPt.x - inextPt.x;
                b = nextPt.y - inextPt.y;
                w00 = (1.f - a)*(1.f - b); w01 = a*(1.f - b);
                w10 = (1.f - a)*b; w11 = a*b;

                double b1 = 0, b2 = 0, ib1 = 0, ib2 = 0;

                for( y = 0; y < winSize.height; y++ )
                {
                    const float* src = (const float*)(derivJ.data +
                        (y + inextPt.y)*derivJ.step) + inextPt.x*cnJ;
                    const float* Ibuf = (float*)(derivIWinBuf.data + y*derivIWinBuf.step);

                    for( x = 0; x < winSize.width; x++, src += cnJ, Ibuf += cnI )
                    {
                        double It = src[0]*w00 + src[cnJ]*w01 + src[stepJ]*w10 +
                                    src[stepJ+cnJ]*w11 - Ibuf[0];
                        double Ixt = src[1]*w00 + src[cnJ+1]*w01 + src[stepJ+1]*w10 +
                                     src[stepJ+cnJ+1]*w11 - Ibuf[1];
                        double Iyt = src[2]*w00 + src[cnJ+2]*w01 + src[stepJ+2]*w10 +
                                     src[stepJ+cnJ+2]*w11 - Ibuf[2];
                        b1 += Ixt*Ibuf[3] + Iyt*Ibuf[4];
                        b2 += Ixt*Ibuf[4] + Iyt*Ibuf[5];
                        ib1 += It*Ibuf[1];
                        ib2 += It*Ibuf[2];
                    }
                }

                b1 = lambda1*ib1 + lambda2*b1;
                b2 = lambda1*ib2 + lambda2*b2;
                Point2f delta( (float)((A12*b2 - A22*b1) * D),
                               (float)((A12*b1 - A11*b2) * D));
                //delta = -delta;

                nextPt += delta;
                nextPts[ptidx] = nextPt + halfWin;

                if( delta.ddot(delta) <= criteria.epsilon )
                    break;

                if( j > 0 && std::abs(delta.x + prevDelta.x) < 0.01 &&
                    std::abs(delta.y + prevDelta.y) < 0.01 )
                {
                    nextPts[ptidx] -= delta*0.5f;
                    break;
                }
                prevDelta = delta;
            }
        }
    }
}

}

/*
struct Point2f {
  float x, y;
  Point2f(float x0, float y0) { x = x0, y = y0; }
};
*/


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
  /*
  cvFindCornerSubPix(
    imgA,
    cornersA,
    corner_count,
    cvSize(win_size, win_size),
    cvSize(-1, -1),
    cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS, 20, 0.03)
  );
  */

  // Call the Lucas-Kanade algorithm
  int flags = CV_LKFLOW_PYR_A_READY;
  CvSize pyr_sz = cvSize( imgA->width+8, imgB->height/3 );
  if( !pyrA || !pyrB ) {
    pyrA = cvCreateImage( pyr_sz, IPL_DEPTH_32F, 1 );
    pyrB = cvCreateImage( pyr_sz, IPL_DEPTH_32F, 1 );
    flags = 0;
  }

  /*
  test_cvCalcOpticalFlowPyrLK(
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
  */
  vector <cv::Point2f> prevPts, nextPts;

  for( int i = 0; i < corner_count; i++ )
    prevPts.push_back(cv::Point2f(cornersA[i].x, cornersA[i].y));

  vector <uchar> status;
  vector <float> err;

  cv::test_calcOpticalFlowPyrLK(
    imgA,
    imgB,
    prevPts,
    nextPts,
    status,
    err,
    cvSize( win_size, win_size ),
    0, // levels
    cvTermCriteria( CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 20, .3 )
  );

  for( int i = 0; i < corner_count; i++ )
    cornersB[i].x = nextPts[i].x, cornersB[i].y = nextPts[i].y;

  // Draw resulting velocity vectors
  for( int i = 0; i < corner_count; i++ ) {
    // if( features_found[i] == 0 || feature_errors[i] > 550 ) {
    if( status[i] == 0 || err[i] > 550 ) {
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
    /*
    if(hypotenuse < 1.01)
      hypotenuse = 1.01;
    if(hypotenuse > 1.99)
      hypotenuse = 1.99;
    */
    hypotenuse = 1.5;
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

  for(int i = 0; i < (1 << 30); i++) {
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

    char c = cvWaitKey( 0 ); // ms to wait
    if( c == 27 ) // ESC key
      break;
  }

  cvWaitKey(0);
  cvReleaseCapture( &g_capture );
  cvDestroyWindow( "Example Video" );

  return 0;
}

