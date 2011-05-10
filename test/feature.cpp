
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



static void
calcMinEigenVal( const Mat& _cov, Mat& _dst )
{
    int i, j;
    Size size = _cov.size();

    for( i = 0; i < size.height; i++ )
    {
        const float* cov = (const float*)(_cov.data + _cov.step*i);
        float* dst = (float*)(_dst.data + _dst.step*i);
        for( j = 0; j < size.width; j++ )
        {
            double a = cov[j*3]*0.5;
            double b = cov[j*3+1];
            double c = cov[j*3+2]*0.5;
            dst[j] = (float)((a + c) - std::sqrt((a - c)*(a - c) + b*b));
        }
    }
}

static void
cornerEigenValsVecs( int H, int W, char *src, const Mat& src_orig,
                      Mat& eigenv, int block_size,
                     int aperture_size, int op_type, double k=0.,
                     int borderType=BORDER_DEFAULT )
{
    /*
    int depth = src.depth();
    double scale = (double)(1 << ((aperture_size > 0 ? aperture_size : 3) - 1)) * block_size;
    // 1 << (3 - 1) = 4
    // 4 * 3 = 12
    if( depth == CV_8U )
        scale *= 255.;
    printf("%.2lf\n", scale);
    scale = 1./scale;
    */

    // block_size = 3, aperture_size = 3, pixel depth 8U
    double scale = 1.0 / (255.0 * 12); 

//    CV_Assert( src.type() == CV_8UC1 || src.type() == CV_32FC1 );

    int i, j;

/*
    Mat Dx, Dy, src2;
    src2.create( cvSize( W, H ), CV_32F );
    printf("src2: %d, W: %d\n", src2.step, W);
    for (i = 0; i < H; i++) {
      float *src3 = (float *) (src2.data + i*src2.step);
      for (j = 0; j < W; j++)
        src3[j] = (float ) src[i*W+j] / 255.0;

      //if(i > 100 && i < 103) {
      //  printf("- %.4lf %.4lf %.4lf\n", src3[10], src3[11], src3[12]);
      //  const char *src4 = (const char *) (src_orig.data + i*src_orig.step);
      //  printf("> %.4lf %.4lf %.4lf\n", src4[10], src4[11], src4[12]);
      //}
    }
    Sobel( src_orig, Dx, CV_32F, 1, 0, aperture_size, scale, 0, borderType );
    Sobel( src_orig, Dy, CV_32F, 0, 1, aperture_size, scale, 0, borderType );

    for( i = 200; i < 203; i++)
      for (j = 500; j < 505; j++) {
        const float *dxdata = (const float *) (Dx.data + i*Dx.step);
        const float *dydata = (const float *) (Dy.data + i*Dy.step);
        printf("> %.4lf %.4lf\n", dxdata[j], dydata[j]);
      }
      */

    // need 32F type - TODO
    float *Dx = (float *) malloc(H * W * sizeof(float));
    float *Dy = (float *) malloc(H * W * sizeof(float));
    // smooth?

    for( i = 1; i < H - 1; i++ ) {
      uint8_t *l0 = (uint8_t *) (src + (i-1)*W);
      uint8_t *l1 = (uint8_t *) (src + (i)*W);
      uint8_t *l2 = (uint8_t *) (src + (i+1)*W);
      for( j = 1; j < W - 1; j++ ) {
        float dx = l0[j+1] + 2*l1[j+1] + l2[j+1] - l0[j-1] - 2*l1[j-1] - l2[j-1];
        float dy = l2[j-1] + 2*l2[j] + l2[j+1] - l0[j+1] - 2*l0[j] - l0[j-1];
        /*
        float dx = src[l0+j+1] + 2*src[l1+j+1] + src[l2+j+1]
                 - src[l0+j-1] - 2*src[l1+j-1] - src[l2+j-1];
        float dy = src[l2+j-1] + 2*src[l2+j] + src[l2+j+1]
                 - src[l0+j-1] - 2*src[l0+j] - src[l0+j+1];
        */

        Dx[i*W+j] = dx * scale;
        Dy[i*W+j] = dy * scale;
        //if(dx > 2 && dy > 2) {
        //  printf("%d %d %.2lf %.2lf | %.4lf %.4lf\n", i, j, dx, dy, Dx[l1+j], Dy[l1+j]);
        //  printf(" >>> %.4lf %.4lf\n", Dx2.data[i*Dx2.step+j], Dy2.data[i*Dy2.step+j]);
        //}
      }
      // borders
      Dx[i*W] = Dy[i*W] = 0;
      Dx[i*W+W-1] = Dy[i*W+W-1] = 0;
    }
    for( j = 0; j < W; j++) {
      // borders
      Dx[j] = Dy[j] = 0;
      Dx[(H-1)*W+j] = Dy[(H-1)*W+j] = 0;
    }

/*
    for( i = 200; i < 203; i++)
      for (j = 500; j < 505; j++) {
        const float *dxdata = (const float *) (Dx + i*W);
        const float *dydata = (const float *) (Dy + i*W);
        printf("> %.4lf %.4lf\n", dxdata[j], dydata[j]);
      }
      */


    //Size size = src.size();
    Size size = cvSize( W, H );
    Mat cov( size, CV_32FC3 );

    for( i = 0; i < H; i++ )
    {
        float* cov_data = (float*)(cov.data + i*cov.step);
        /*
        const float* dxdata = (const float*)(Dx.data + i*Dx.step);
        const float* dydata = (const float*)(Dy.data + i*Dy.step);
        */
        const float* dxdata = (const float*)(Dx + i*W);
        const float* dydata = (const float*)(Dy + i*W);

        for( j = 0; j < W; j++ )
        {
            float dx = dxdata[j];
            float dy = dydata[j];
            //printf("%d %d: %.6lf %.6lf\n", i, j, dx, dy);

            cov_data[j*3] = dx*dx;
            cov_data[j*3+1] = dx*dy;
            cov_data[j*3+2] = dy*dy;
        }
    }
    // cleanup
    free( Dx );
    free( Dy );

    // TODO
    boxFilter(cov, cov, cov.depth(), Size(block_size, block_size),
        Point(-1,-1), false, borderType ); // normalize = false

//    if( op_type == MINEIGENVAL )
        calcMinEigenVal( cov, eigenv );
        /*
    else if( op_type == HARRIS )
        calcHarris( cov, eigenv, k );
    else if( op_type == EIGENVALSVECS )
        calcEigenValsVecs( cov, eigenv );
        */
}

void test_cornerMinEigenVal( int H, int W, char *src, const Mat& src_orig,
                          Mat& dst, int blockSize, int ksize, int borderType )
{
    printf("HERE\n");
    dst.create( cvSize( W, H ), CV_32F );
    cornerEigenValsVecs( H, W, src, src_orig,
                          dst, blockSize, ksize, 0/*MINEIGENVAL*/, 0, borderType );
}



void features( IplImage *image ) {
  int H = image->height;
  int W = image->width;
  char *src = image->imageData;

  int maxCorners = 100;
  double qualityLevel = 0.03;
  double minDistance = 5.0;
  Mat mask;
  int blockSize = 3;

  Mat eig, tmp;
  test_cornerMinEigenVal( H, W, src, image, eig, blockSize, 3, BORDER_DEFAULT ); // TODO - tough part

  double maxVal = 0;
  //  threshold( eig, eig, maxVal*qualityLevel, 0, THRESH_TOZERO ); // TODO
  for( int y = 1; y < H - 1; y++ ) {
    for( int x = 1; x < W - 1; x++ ) {
      float val = eig.at<float>(y, x);
      if( val > maxVal )
        maxVal = val;
    }
  }

  //  minMaxLoc( eig, 0, &maxVal, 0, 0, mask ); // TODO
  double threshold = maxVal * qualityLevel;
  for( int y = 1; y < H - 1; y++ ) {
    for( int x = 1; x < W - 1; x++ ) {
      float val = eig.at<float>(y, x);
      if(val < threshold)
        eig.at<float>(y, x) = 0;
    }
  }

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

