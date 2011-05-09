
#include "highgui.h"
#include "cv.h"

#include <algorithm>
#include <cstdio>
#include <vector>

//#define USE_OPENCV

#define zmin(x, y) ((x) < (y) ? (x) : (y))
#define zmax(x, y) ((x) > (y) ? (x) : (y))

using namespace cv;

const int MAX_CORNERS = 100;
int corner_count = MAX_CORNERS;

CvPoint2D32f cornersA[ MAX_CORNERS ];

int greaterThanFloatPtr(const void *a, const void *b) {
  float x = **(float **)a;
  float y = **(float **)b;
  if(x > y) return -1;
  if(x < y) return 1;
  return 0;
}

// very basic vector classes
struct fpvector {
  size_t cursize, maxsize;
  float **data;

  fpvector() {
    cursize = 0;
    maxsize = 16;
    data = (float **) malloc(maxsize * sizeof(float *));
  }

  void push_back(float *a) {
    if(cursize == maxsize) {
      maxsize *= 2;
      data = (float **) realloc(data, maxsize * sizeof(float *));
    }
    data[cursize++] = a;
  }

  float* operator[](const int i) const { return data[i]; }

  size_t size() { return cursize; }
};

struct point {
  float x, y;
  point(float x0, float y0) { x = x0; y = y0; }
};

struct ptvector {
  size_t cursize, maxsize;
  point *data;

  ptvector() {
    cursize = 0;
    maxsize = 8;
    data = (point *) malloc(maxsize * sizeof(point));
  }
  void push_back(point a) {
    if(cursize == maxsize) {
      maxsize *= 2;
      data = (point *) realloc(data, maxsize * sizeof(point));
    }
    data[cursize++] = a;
  }
  point operator[](const int i) const { return data[i]; }
  size_t size() { return cursize; }
};

void features( IplImage *image ) {
  int H = image->height;
  int W = image->width;
  /*
  char *src = img->imageData;
  */

  int maxCorners = 100;
  double qualityLevel = 0.03;
  double minDistance = 5.0;
  Mat mask;
  int blockSize = 3;

  Mat eig, tmp;
  cornerMinEigenVal( image, eig, blockSize, 3 ); // TODO - tough part

  double maxVal = 0;
  minMaxLoc( eig, 0, &maxVal, 0, 0, mask ); // TODO
//  threshold( eig, eig, maxVal*qualityLevel, 0, THRESH_TOZERO ); // TODO
  /*
  double threshold = maxVal * qualityLevel;
  for( int y = 1; y < H - 1; y++ ) {
    for( int x = 1; x < W - 1; x++ ) {
      float val = eig.at<float>(y, x);
      if(val < 0)
        eig.at<float>(y, x) = 0;
    }
  }
  */
  dilate( eig, tmp, Mat()); // TODO

  fpvector tmpCorners;

  // collect list of pointers to features - put them into temporary image
  for( int y = 1; y < H - 1; y++ )
  {
    float* eig_data = (float*)eig.ptr(y);
    const float* tmp_data = (const float*)tmp.ptr(y);
    const uchar* mask_data = mask.data ? mask.ptr(y) : 0;

    for( int x = 1; x < W - 1; x++ )
    {
      float val = eig_data[x];
      if( val != 0 && val == tmp_data[x] && (!mask_data || mask_data[x]) )
        tmpCorners.push_back(eig_data + x);
    }
  }

  size_t i, j, total = tmpCorners.size(), ncorners = 0;

  // sort features
  qsort( tmpCorners.data, total, sizeof(float *), greaterThanFloatPtr );

  // Partition the image into larger grids
  const int cell_size = (int) (minDistance + 0.5);
  const int grid_width = (W + cell_size - 1) / cell_size;
  const int grid_height = (H + cell_size - 1) / cell_size;

  ptvector grid[grid_width * grid_height];

  minDistance *= minDistance;

  for( i = 0; i < total; i++ )
  {
    int ofs = (int)((const uchar*)tmpCorners[i] - eig.data);
    int y = (int)(ofs / eig.step);
    int x = (int)((ofs - y*eig.step)/sizeof(float));

    bool good = true;

    int x_cell = x / cell_size;
    int y_cell = y / cell_size;

    int x1 = x_cell - 1;
    int y1 = y_cell - 1;
    int x2 = x_cell + 1;
    int y2 = y_cell + 1;

    // boundary check
    x1 = zmax(0, x1);
    y1 = zmax(0, y1);
    x2 = zmin(grid_width-1, x2);
    y2 = zmin(grid_width-1, y2);

    for( int yy = y1; yy <= y2; yy++ )
    {
      for( int xx = x1; xx <= x2; xx++ )
      {   
        ptvector &m = grid[yy*grid_width + xx];

        if( m.size() )
        {
          for(j = 0; j < m.size(); j++)
          {
            float dx = x - m[j].x;
            float dy = y - m[j].y;

            if( dx*dx + dy*dy < minDistance )
            {
              good = false;
              goto break_out;
            }
          }
        }                
      }
    }

break_out:

    if(good)
    {
      // printf("%d: %d %d -> %d %d, %d, %d -- %d %d %d %d, %d %d, c=%d\n",
      //    i,x, y, x_cell, y_cell, (int)minDistance, cell_size,x1,y1,x2,y2, grid_width,grid_height,c);
      grid[y_cell*grid_width + x_cell].push_back(point((float)x, (float)y));

      cornersA[ncorners++] = Point2f((float)x, (float)y);

        if( maxCorners > 0 && (int)ncorners == maxCorners )
          break;
      }
    }
}

int main( int argc, char** argv ) {
  if(argc != 2) {
    printf("Usage: ./feature <image name>\n");
    return 0;
  }

  // Show original image
  IplImage* inImg = cvLoadImage( argv[1] );
  // Create image with 8U pixel depth, single color channel
  IplImage* img = cvCreateImage(
      cvSize(inImg->width, inImg->height),
      IPL_DEPTH_8U, 1
  );
  // Convert and display
  cvCvtColor(inImg, img, CV_BGR2GRAY);
  cvConvertScale(img, img);
  cvNamedWindow( "Input", CV_WINDOW_AUTOSIZE );
  cvShowImage( "Input", img );

  // Run feature detection
#ifdef USE_OPENCV
  // OpenCV implementation
  CvSize img_sz = cvGetSize( img );
  IplImage* eig_image = cvCreateImage( img_sz, IPL_DEPTH_32F, 1 );
  IplImage* tmp_image = cvCreateImage( img_sz, IPL_DEPTH_32F, 1 );

  cvGoodFeaturesToTrack(
    img,
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
#else
  features( img );
  printf("good so far\n");
#endif

  printf("corners: %d\n", corner_count);
  // Mark features
  for( int i = 0; i < corner_count; i++ ) {
    double x = cornersA[i].x;
    double y = cornersA[i].y;
    //printf("%.2lf %.2lf\n", x, y);
    CvPoint p = cvPoint( cvRound(x), cvRound(y) );
    CvScalar color = CV_RGB(255, 0, 0);
    cvCircle( inImg, p, 5, color, 2 );
  }

  // Display
  cvNamedWindow( "Output", CV_WINDOW_AUTOSIZE );
  cvShowImage( "Output", inImg );

  // Cleanup
  cvWaitKey(0);
  cvReleaseImage( &img );
  cvDestroyWindow( "Input" );
  cvDestroyWindow( "Output" );

  return 0;
}

