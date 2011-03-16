#include "highgui.h"
#include "cv.h"
#include <cstdio>

//#define USE_OPENCV

int main( int argc, char** argv ) {
  if(argc != 2) {
    printf("Usage: ./canny <image name>\n");
    return 0;
  }

  // Show original image
  IplImage* img = cvLoadImage( argv[1], 0 );
  cvNamedWindow( "Input", CV_WINDOW_AUTOSIZE );
  cvShowImage( "Input", img );
  printf("Input depth: %x\n", img->depth); // 8U

  // Run Canny edge detection
  IplImage *res = cvCreateImage( cvGetSize(img), IPL_DEPTH_8U, 1 );
#ifdef USE_OPENCV
  // OpenCV implementation
  cvCanny( img, res, 10, 100 );
#else
  // Own implementation of Sobel kernel
  int H = img->height, W = img->width, WS = img->widthStep;
  uint8_t *srcdata = (uint8_t *) img->imageData;
  uint8_t *dstdata = (uint8_t *) res->imageData;
  // TODO - clean up
  double low_thresh = 10;
  double high_thresh = 100;

  // TODO - replacements
  // - cv::Mat with char*
  // - CvSize with H and W
  // - stack with C implementation
  cv::Ptr<CvMat> dx, dy;
  cv::AutoBuffer<char> buffer;
  std::vector<uchar*> stack;
  uchar **stack_top = 0, **stack_bottom = 0;

//  CvMat srcstub, *src = img;//cvGetMat( srcarr, &srcstub );
//  CvMat dststub, *dst = res;//cvGetMat( dstarr, &dststub );
  CvSize size;
  int aperture_size = 3;
  int flags = aperture_size;
  int low, high;
  int* mag_buf[3];
  uchar* map;
  int mapstep, maxsize;
  int i, j;
  CvMat mag_row;

//  size = cvGetMatSize( src );
  size = cvGetSize( img );

  dx = cvCreateMat( size.height, size.width, CV_16SC1 );
  dy = cvCreateMat( size.height, size.width, CV_16SC1 );
  cvSobel( img, dx, 1, 0, aperture_size );
  cvSobel( img, dy, 0, 1, aperture_size );


  if( flags & CV_CANNY_L2_GRADIENT )
  {
    Cv32suf ul, uh;
    ul.f = (float)low_thresh;
    uh.f = (float)high_thresh;

    low = ul.i;
    high = uh.i;
  }
  else
  {
    low = cvFloor( low_thresh );
    high = cvFloor( high_thresh );
  }

  buffer.allocate( (size.width+2)*(size.height+2) + (size.width+2)*3*sizeof(int) );

  mag_buf[0] = (int*)(char*)buffer;
  mag_buf[1] = mag_buf[0] + size.width + 2;
  mag_buf[2] = mag_buf[1] + size.width + 2;
  map = (uchar*)(mag_buf[2] + size.width + 2);
  mapstep = size.width + 2;

  maxsize = MAX( 1 << 10, size.width*size.height/10 );
  stack.resize( maxsize );
  stack_top = stack_bottom = &stack[0];

  memset( mag_buf[0], 0, (size.width+2)*sizeof(int) );
  memset( map, 1, mapstep );
  memset( map + mapstep*(size.height + 1), 1, mapstep );

  /* sector numbers 
     (Top-Left Origin)

     1   2   3
   *  *  * 
   * * *  
   0*******0
   * * *  
   *  *  * 
   3   2   1
   */

#define CANNY_PUSH(d)    *(d) = (uchar)2, *stack_top++ = (d)
#define CANNY_POP(d)     (d) = *--stack_top

  mag_row = cvMat( 1, size.width, CV_32F );

  // calculate magnitude and angle of gradient, perform non-maxima supression.
  // fill the map with one of the following values:
  //   0 - the pixel might belong to an edge
  //   1 - the pixel can not belong to an edge
  //   2 - the pixel does belong to an edge
  for( i = 0; i <= size.height; i++ )
  {
    int* _mag = mag_buf[(i > 0) + 1] + 1;
    float* _magf = (float*)_mag;
    const short* _dx = (short*)(dx->data.ptr + dx->step*i);
    const short* _dy = (short*)(dy->data.ptr + dy->step*i);
    uchar* _map;
    int x, y;
    int magstep1, magstep2;
    int prev_flag = 0;

    if( i < size.height )
    {
      _mag[-1] = _mag[size.width] = 0;

      // Use L1 norm
      for( j = 0; j < size.width; j++ )
        _mag[j] = abs(_dx[j]) + abs(_dy[j]);
    }
    else
      memset( _mag-1, 0, (size.width + 2)*sizeof(int) );

    // at the very beginning we do not have a complete ring
    // buffer of 3 magnitude rows for non-maxima suppression
    if( i == 0 )
      continue;

    _map = map + mapstep*i + 1;
    _map[-1] = _map[size.width] = 1;

    _mag = mag_buf[1] + 1; // take the central row
    _dx = (short*)(dx->data.ptr + dx->step*(i-1));
    _dy = (short*)(dy->data.ptr + dy->step*(i-1));

    magstep1 = (int)(mag_buf[2] - mag_buf[1]);
    magstep2 = (int)(mag_buf[0] - mag_buf[1]);

    if( (stack_top - stack_bottom) + size.width > maxsize )
    {
      int sz = (int)(stack_top - stack_bottom);
      maxsize = MAX( maxsize * 3/2, maxsize + 8 );
      stack.resize(maxsize);
      stack_bottom = &stack[0];
      stack_top = stack_bottom + sz;
    }

    for( j = 0; j < size.width; j++ )
    {
#define CANNY_SHIFT 15
#define TG22  (int)(0.4142135623730950488016887242097*(1<<CANNY_SHIFT) + 0.5)

      x = _dx[j];
      y = _dy[j];
      int s = x ^ y;
      int m = _mag[j];

      x = abs(x);
      y = abs(y);
      if( m > low )
      {
        int tg22x = x * TG22;
        int tg67x = tg22x + ((x + x) << CANNY_SHIFT);

        y <<= CANNY_SHIFT;

        if( y < tg22x )
        {
          if( m > _mag[j-1] && m >= _mag[j+1] )
          {
            if( m > high && !prev_flag && _map[j-mapstep] != 2 )
            {
              CANNY_PUSH( _map + j );
              prev_flag = 1;
            }
            else
              _map[j] = (uchar)0;
            continue;
          }
        }
        else if( y > tg67x )
        {
          if( m > _mag[j+magstep2] && m >= _mag[j+magstep1] )
          {
            if( m > high && !prev_flag && _map[j-mapstep] != 2 )
            {
              CANNY_PUSH( _map + j );
              prev_flag = 1;
            }
            else
              _map[j] = (uchar)0;
            continue;
          }
        }
        else
        {
          s = s < 0 ? -1 : 1;
          if( m > _mag[j+magstep2-s] && m > _mag[j+magstep1+s] )
          {
            if( m > high && !prev_flag && _map[j-mapstep] != 2 )
            {
              CANNY_PUSH( _map + j );
              prev_flag = 1;
            }
            else
              _map[j] = (uchar)0;
            continue;
          }
        }
      }
      prev_flag = 0;
      _map[j] = (uchar)1;
    }

    // scroll the ring buffer
    _mag = mag_buf[0];
    mag_buf[0] = mag_buf[1];
    mag_buf[1] = mag_buf[2];
    mag_buf[2] = _mag;
  }

  // now track the edges (hysteresis thresholding)
  while( stack_top > stack_bottom )
  {
    uchar* m;
    if( (stack_top - stack_bottom) + 8 > maxsize )
    {
      int sz = (int)(stack_top - stack_bottom);
      maxsize = MAX( maxsize * 3/2, maxsize + 8 );
      stack.resize(maxsize);
      stack_bottom = &stack[0];
      stack_top = stack_bottom + sz;
    }

    CANNY_POP(m);

    if( !m[-1] )
      CANNY_PUSH( m - 1 );
    if( !m[1] )
      CANNY_PUSH( m + 1 );
    if( !m[-mapstep-1] )
      CANNY_PUSH( m - mapstep - 1 );
    if( !m[-mapstep] )
      CANNY_PUSH( m - mapstep );
    if( !m[-mapstep+1] )
      CANNY_PUSH( m - mapstep + 1 );
    if( !m[mapstep-1] )
      CANNY_PUSH( m + mapstep - 1 );
    if( !m[mapstep] )
      CANNY_PUSH( m + mapstep );
    if( !m[mapstep+1] )
      CANNY_PUSH( m + mapstep + 1 );
  }

  // the final pass, form the final image
  for( i = 0; i < size.height; i++ )
  {
    const uchar* _map = map + mapstep*(i+1) + 1;
    uchar* _dst = dstdata + res->widthStep*i;

    for( j = 0; j < size.width; j++ )
      _dst[j] = (uchar)-(_map[j] >> 1);
  }

#endif

  // Display
  cvNamedWindow( "Output", CV_WINDOW_AUTOSIZE );
  cvShowImage( "Output", res );

  // Cleanup
  cvWaitKey(0);
  cvReleaseImage( &img );
  cvReleaseImage( &res );
  cvDestroyWindow( "Input" );
  cvDestroyWindow( "Output" );

  return 0;
}

