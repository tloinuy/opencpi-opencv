#include "highgui.h"
#include "cv.h"
#include <cstdio>

#define USE_OPENCV

int main( int argc, char** argv ) {
  if(argc != 2) {
    printf("Usage: ./canny <image name>\n");
    return 0;
  }

  // Show original image
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

  double low_thresh = 10;
  double high_thresh = 100;

  uchar **stack = 0;
  uchar **stack_top = 0, **stack_bottom = 0;

  int aperture_size = 3;
  int flags = aperture_size;
  int low, high;
  int* mag_buf[3];
  uchar* map;
  int mapstep, maxsize;
  int i, j;

  // Sobel kernels for calculating gradients
  uint16_t *dx = (uint16_t *) malloc( H * W * sizeof(uint16_t) );
  uint16_t *dy = (uint16_t *) malloc( H * W * sizeof(uint16_t) );
#define ind(i,j) ((i)*W+(j))
  for(i = 1; i + 1 < H; i++) {
    for(j = 1; j + 1 < W; j++) {
      dx[ind(i, j)] = srcdata[ind(i-1, j+1)] - srcdata[ind(i-1, j-1)]
                  + 2 * ( srcdata[ind(i, j+1)] - srcdata[ind(i, j-1)] )
                  + srcdata[ind(i+1, j+1)] - srcdata[ind(i+1, j-1)];
      dy[ind(i, j)] = srcdata[ind(i+1, j-1)] - srcdata[ind(i-1, j-1)]
                  + 2 * ( srcdata[ind(i+1, j)] - srcdata[ind(i-1, j)] )
                  + srcdata[ind(i+1, j+1)] - srcdata[ind(i-1, j+1)];
    }
  }
  for(i = 0; i < H; i++)
    dx[ind(i, 0)] = dx[ind(i, W-1)] = dy[ind(i, 0)] = dy[ind(i, W-1)] = 0;
  for(j = 0; j < W; j++)
    dx[ind(0, j)] = dx[ind(H-1, j)] = dy[ind(0, j)] = dy[ind(H-1, j)] = 0;

#undef ind

  // setting thresholds
  low = (int) low_thresh;
  high = (int) high_thresh;

  char *buffer;
  buffer = (char *) malloc( (W+2)*(H+2) + (W+2)*3*sizeof(int) );

  mag_buf[0] = (int*)buffer;
  mag_buf[1] = mag_buf[0] + W + 2;
  mag_buf[2] = mag_buf[1] + W + 2;
  map = (uchar*)(mag_buf[2] + W + 2);
  mapstep = W + 2;

  // allocate stack directly
  maxsize = H * W;
  stack = (uchar **) malloc( maxsize*sizeof(uchar) );
  stack_top = stack_bottom = &stack[0];

  memset( mag_buf[0], 0, (W+2)*sizeof(int) );
  memset( map, 1, mapstep );
  memset( map + mapstep*(H + 1), 1, mapstep );

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


  // calculate magnitude and angle of gradient, perform non-maxima supression.
  // fill the map with one of the following values:
  //   0 - the pixel might belong to an edge
  //   1 - the pixel can not belong to an edge
  //   2 - the pixel does belong to an edge
  for( i = 0; i <= H; i++ )
  {
    int* _mag = mag_buf[(i > 0) + 1] + 1;
    float* _magf = (float*)_mag;
    const short* _dx = (short*)(dx + W*i);
    const short* _dy = (short*)(dy + W*i);
    uchar* _map;
    int x, y;
    int magstep1, magstep2;
    int prev_flag = 0;

    if( i < H )
    {
      _mag[-1] = _mag[W] = 0;

      // Use L1 norm
      for( j = 0; j < W; j++ )
        _mag[j] = abs(_dx[j]) + abs(_dy[j]);
    }
    else
      memset( _mag-1, 0, (W + 2)*sizeof(int) );

    // at the very beginning we do not have a complete ring
    // buffer of 3 magnitude rows for non-maxima suppression
    if( i == 0 )
      continue;

    _map = map + mapstep*i + 1;
    _map[-1] = _map[W] = 1;

    _mag = mag_buf[1] + 1; // take the central row
    _dx = (short*)(dx + W*(i-1));//(dx->data.ptr + dx->step*(i-1));
    _dy = (short*)(dy + W*(i-1));//(dy->data.ptr + dy->step*(i-1));

    magstep1 = (int)(mag_buf[2] - mag_buf[1]);
    magstep2 = (int)(mag_buf[0] - mag_buf[1]);

    for( j = 0; j < W; j++ )
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
  for( i = 0; i < H; i++ )
  {
    const uchar* _map = map + mapstep*(i+1) + 1;
    uchar* _dst = dstdata + res->widthStep*i;

    for( j = 0; j < W; j++ )
      _dst[j] = (uchar)-(_map[j] >> 1);
  }

  // Cleanup
  free( stack );
  free( buffer );

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

