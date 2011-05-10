/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Tue April 26 01:56:18 2011 EDT
 * BASED ON THE FILE: good_features_to_track.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: good_features_to_track
 */
#include "good_features_to_track_Worker.h"
#include <string.h>
#include <stdlib.h>

GOOD_FEATURES_TO_TRACK_METHOD_DECLARATIONS;
RCCDispatch good_features_to_track = {
  /* insert any custom initializations here */
  GOOD_FEATURES_TO_TRACK_DISPATCH
};

#define FRAME_BYTES (p->height * p->width * sizeof(Pixel)) // pixels per frame

// properties (and defaults)

// simple routines: OpenCV and STL substitutes
#define zmin(x, y) ((x) < (y) ? (x) : (y))
#define zmax(x, y) ((x) > (y) ? (x) : (y))

int greaterThanFloatPtr(const void *a, const void *b) {
  float x = **(float **)a;
  float y = **(float **)b;
  if(x > y) return -1;
  if(x < y) return 1;
  return 0;
}

/*
// very basic vector classes
struct fpvector {
  size_t cursize, maxsize;
  float **data;
} tmpCorners;

void fpvector_init() {
  cursize = 0;
  maxsize = 16;
  data = (float **) malloc(maxsize * sizeof(float *));
}

void tmpCorners_push_back(float *a) {
  if(tmpCorners.cursize == tmpCorners.maxsize) {
    tmpCorners.maxsize *= 2;
    tmpCorners.data = (float **) realloc(
              tmpCorners.data, tmpCorners.maxsize * sizeof(float *));
  }
  tmpCorners.data[tmpCorners.cursize++] = a;
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
  struct point *data;

  ptvector() {
    cursize = 0;
    maxsize = 8;
    data = (struct point *) malloc(maxsize * sizeof(struct point));
  }
  void push_back(struct point a) {
    if(cursize == maxsize) {
      maxsize *= 2;
      data = (struct point *) realloc(data, maxsize * sizeof(struct point));
    }
    data[cursize++] = a;
  }
  struct point operator[](const int i) const { return data[i]; }
  size_t size() { return cursize; }
};
*/

// main loop
static size_t calc_features(int H, int W, float *src, float *dst,
            int maxCorners, double qualityLevel, double minDistance)
{
  // int H = image->height;
  // int W = image->width;
  // char *src = image->imageData;

  // int maxCorners = 100;
  if(maxCorners < 0)
    maxCorners = 100;
  // double qualityLevel = 0.03;
  if(qualityLevel < 0)
    qualityLevel = 0.03;
  // double minDistance = 7.0;
  if(minDistance < 0)
    minDistance = 7.0;
  // Mat mask;
  // int blockSize = 3;

  // Mat eig, tmp;
  // float *eig = (float *) malloc(H * W * sizeof(float));
  float *eig = src;
  float *tmp = (float *) malloc(H * W * sizeof(float));
  // test_cornerMinEigenVal( H, W, src, eig); //, blockSize, 3, BORDER_DEFAULT );

  int x, y;
  double maxVal = 0;
  // minMaxLoc( eig, 0, &maxVal, 0, 0, mask );
  for( y = 0; y < H; y++ ) {
    for( x = 0; x < W; x++ ) {
      float val = eig[y*W+x]; // eig.at<float>(y, x);
      if( val > maxVal )
        maxVal = val;
    }
  }

  double threshold = maxVal * qualityLevel;
  // threshold( eig, eig, maxVal*qualityLevel, 0, THRESH_TOZERO );
  for( y = 0; y < H; y++ ) {
    for( x = 0; x < W; x++ ) {
      float val = eig[y*W+x]; // eig.at<float>(y, x);
      if(val < threshold)
        eig[y*W+x] = 0;
        // eig.at<float>(y, x) = 0;
    }
  }

  // dilate( eig, tmp, Mat());
  for( y = 1; y < H - 1; y++ ) {
    for( x = 1; x < W - 1; x++ ) {
      float val = eig[y*W+x];
      int dx, dy;
      for( dy = -1; dy <= 1; dy++ )
        for( dx = -1; dx <= 1; dx++ )
          val = zmax(eig[(y+dy)*W+x+dx], val);

      tmp[y*W+x] = val;
    }
    // border
    tmp[y*W] = eig[y*W];
    tmp[y*W+W-1] = eig[y*W+W-1];
  }
  // border
  for( x = 0; x < W; x++ ) {
    tmp[x] = eig[x];
    tmp[(H-1)*W+x] = eig[(H-1)*W+x];
  }


  float **tmpCorners = malloc((H-1)*(W-1)*sizeof(float *));
  size_t tmpCornersSize = 0;

  // collect list of pointers to features - put them into temporary image
  for( y = 1; y < H - 1; y++ )
  {
    // float* eig_data = (float*)eig.ptr(y);
    // const float* tmp_data = (const float*)tmp.ptr(y);
    float* eig_data = eig + y*W;
    const float* tmp_data = (const float*) (tmp + y*W);
    // const uchar* mask_data = mask.data ? mask.ptr(y) : 0;

    for( x = 1; x < W - 1; x++ )
    {
      float val = eig_data[x];
      if( val != 0 && val == tmp_data[x]) // && (!mask_data || mask_data[x]) )
        tmpCorners[tmpCornersSize++] = eig_data + x;
        // tmpCorners.push_back(eig_data + x);
    }
  }

  size_t i, j, total = tmpCornersSize, ncorners = 0;

  // sort features
  qsort( tmpCorners, total, sizeof(float *), greaterThanFloatPtr );

  // Partition the image into larger grids
  const int cell_size = (int) (minDistance + 0.5);
  const int grid_width = (W + cell_size - 1) / cell_size;
  const int grid_height = (H + cell_size - 1) / cell_size;

  // struct ptvector grid[grid_width * grid_height];
  float *gridx = malloc(H * W * sizeof(float));
  float *gridy = malloc(H * W * sizeof(float));
  int *gridSize = malloc(grid_height * grid_width * sizeof(int));
  for( i = 0; i < grid_height * grid_width; i++ )
    gridSize[i] = 0;

  minDistance *= minDistance;

  for( i = 0; i < total; i++ )
  {
    /*
    int ofs = (int)((const uchar*)tmpCorners[i] - eig.data);
    int y = (int)(ofs / eig.step);
    int x = (int)((ofs - y*eig.step)/sizeof(float));
    */
    int ofs = (int)(tmpCorners[i] - eig);
    y = (int)(ofs / W);
    x = (int)(ofs - y*W);

    size_t good = 1;

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

    int xx, yy;
    int hw = H * W / (grid_height * grid_width);
    for( yy = y1; yy <= y2; yy++ )
    {
      for( xx = x1; xx <= x2; xx++ )
      {   
        int ind = yy*grid_width + xx;
        // ptvector &m = grid[yy*grid_width + xx];

        // if( m.size() )
        if( gridSize[ind] )
        {
          for(j = 0; j < gridSize[ind]; j++)
          {
            float dx = x - gridx[ind*hw + j];//m[j].x;
            float dy = y - gridy[ind*hw + j];//m[j].y;

            if( dx*dx + dy*dy < minDistance )
            {
              good = 0; // false
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
      // grid[y_cell*grid_width + x_cell].push_back(point((float)x, (float)y));
      int ind = y_cell*grid_width + x_cell;
      gridx[ind*hw + gridSize[ind]] = (float) x;
      gridy[ind*hw + gridSize[ind]] = (float) y;
      gridSize[ind]++;

      dst[2*ncorners] = x; //Point2f((float)x, (float)y);
      dst[2*ncorners+1] = y;
      ncorners++;

        if( maxCorners > 0 && (int)ncorners == maxCorners )
          break;
    }
  }

  // cleanup
  // free( eig );
  free( tmp );
  free( gridx );
  free( gridy );
  free( gridSize );

  return ncorners;
}

/*
 * Methods to implement for worker good_features_to_track, based on metadata.
*/

static RCCResult run(RCCWorker *self,
                     RCCBoolean timedOut,
                     RCCBoolean *newRunCondition) {
  ( void ) timedOut;
  ( void ) newRunCondition;
  Good_features_to_trackProperties *p = self->properties;
  RCCPort *in = &self->ports[GOOD_FEATURES_TO_TRACK_IN],
          *out = &self->ports[GOOD_FEATURES_TO_TRACK_OUT];
  const RCCContainer *c = self->container;

  size_t ncorners = calc_features( p->height, p->width,
                    (float *) in->current.data,
                    (float *) out->current.data,
                    p->max_corners,
                    p->quality_level,
                    p->min_distance );

  out->output.u.operation = in->input.u.operation;
  out->output.length = 2 * ncorners * sizeof(float);

  return RCC_ADVANCE;
}
