#include "highgui.h"
#include "cv.h"
#include <cstdio>

//#define USE_OPENCV

int main( int argc, char** argv ) {
	if(argc != 2) {
		printf("Usage: ./image <image name>\n");
		return 0;
	}

	// Show original image
	IplImage* img = cvLoadImage( argv[1], 0 );
	cvNamedWindow( "Input", CV_WINDOW_AUTOSIZE );
	cvShowImage( "Input", img );
	printf("Input depth: %x\n", img->depth); // 8U

	// Run Sobel edge detection
	IplImage *res = cvCreateImage( cvGetSize(img), IPL_DEPTH_8U, 1 );
#ifdef USE_OPENCV
	// OpenCV implementation
	// Convert images to 16SC1
	IplImage *out = cvCreateImage( cvGetSize(img), IPL_DEPTH_16S, 1 );
	cvSobel( img, out, 1, 0 );
	cvConvertScaleAbs(out, res); // need 8U again
	cvReleaseImage( &out );
#else
	// Own implementation of Sobel kernel
	int H = img->height, W = img->width, WS = img->widthStep;
	uint8_t *srcdata = (uint8_t *) img->imageData;
	uint8_t *dstdata = (uint8_t *) res->imageData;

#define ind(i,j) ((i)*WS+(j))

	// work on bulk of image
	for(int i = 1; i+1 < H; i++) {
		for(int j = 1; j+1 < W; j++) {
			// dx kernel
			// -1 0 1
			// -2 0 2
			// -1 0 1
			int16_t value = - srcdata[ind(i-1,j-1)]
							+ srcdata[ind(i-1,j+1)]
							- srcdata[ind(i,j-1)] * 2
							+ srcdata[ind(i,j+1)] * 2
							- srcdata[ind(i+1,j-1)]
							+ srcdata[ind(i+1,j+1)];
			if(value < 0)
				value = 0;
			if(value > 255)
				value = 255;
			dstdata[ind(i,j)] = value;
		}
	}

	// work on top and bottom
	
	// work on left and right

#undef ind

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

