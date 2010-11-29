
/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
 */



#include <stdio.h>
#include <stdlib.h>
#include <xfer_if.h>
#include <xfer_internal.h>
#include <DtTransferInterface.h>
#include <OcpiOsDataTypes.h>
#include <OcpiOsMisc.h>
#include <DtSharedMemoryInterface.h>

using namespace DataTransfer;

#define USE_BYTE_TRANSFERS
#ifdef USE_BYTE_TRANSFERS
static void
action_pio_transfer(PIO_transfer transfer)
{

  /* Get the alignments */
  OCPI::OS::int64_t src_al = (OCPI::OS::uint64_t)transfer->src_va & 3;
  OCPI::OS::int64_t dst_al = (OCPI::OS::uint64_t)transfer->dst_va & 3;
  OCPI::OS::int32_t i;

  /* Check src and dst alignment */
  if (src_al != dst_al) {
    /* 
     * src and dst addresses don't have same alignment
     * we will need to transfer everything as bytes.
     */
    char *src = (char *)transfer->src_va;
    char *dst = (char *)transfer->dst_va;
    OCPI::OS::int32_t nbytes = transfer->nbytes;


    if (src == 0) {
      for (OCPI::OS::int32_t i=0; i < nbytes; i++)
        *dst++ = 0;
    }
    else {
      for (OCPI::OS::int32_t i=0; i < nbytes; i++)
        *dst++ = *src++;
    }
  }
  else if (src_al > 0) {
    /*
     * We need to perform 'src_al' number of byte transfers
     * before we are word aligned, then we can use word
     * transfers.
     */
    char *src_b = (char *)transfer->src_va;
    char *dst_b = (char *)transfer->dst_va;

    OCPI::OS::int32_t *src_w;
    OCPI::OS::int32_t *dst_w;

    for (i=0; i < src_al; i++)
      *dst_b++ = *src_b++;

    /* Get the word pointers */
    src_w = (OCPI::OS::int32_t *)src_b;
    dst_w = (OCPI::OS::int32_t *)dst_b;

    /* Get the word count and remainder */
    OCPI::OS::int32_t nwords = (transfer->nbytes - src_al) / 4;
    OCPI::OS::int32_t rem_nwords = (transfer->nbytes - src_al) % 4;

    if (src_w == 0) {
      for (i=0; i < nwords; i++)    
        *dst_w++ = 0;
    }
    else {
      for (i=0; i < nwords; i++)    
        *dst_w++ = *src_w++;
    }
    
    if (rem_nwords) {
      /* Set the byte pointers */
      src_b = (char *)src_w;
      dst_b = (char *)dst_w;

      /* Process remainder in bytes */
      if (src_w == 0) {
        for (i=0; i < rem_nwords; i++)
          *dst_b++ = 0;
      }
      else {
        for (i=0; i < rem_nwords; i++)
          *dst_b++ = *src_b++;
      }
    }
  }
  else {
    /* Get the word pointers */
    OCPI::OS::int32_t *src_w = (OCPI::OS::int32_t *)transfer->src_va;
    OCPI::OS::int32_t *dst_w = (OCPI::OS::int32_t *)transfer->dst_va;

    OCPI::OS::int32_t nwords = transfer->nbytes / 4;
    OCPI::OS::int32_t rem_nwords = transfer->nbytes % 4;

    if (src_w == 0) {
      for (i=0; i < nwords; i++)    
        *dst_w++ = 0;
    }
    else {
      for (i=0; i < nwords; i++)    
        *dst_w++ = *src_w++;
    }
    
    if (rem_nwords) {
      /* Set the byte pointers */
      char *src_b = (char *)src_w;
      char *dst_b = (char *)dst_w;

      /* Process remainder in bytes */
      if (src_w == 0) {
        for (i=0; i < rem_nwords; i++)
          *dst_b++ = 0;
      }
      else {
        for (i=0; i < rem_nwords; i++)
          *dst_b++ = *src_b++;
      }
    }
  }

  //#define TRACE_PIO_XFERS  
#ifdef TRACE_PIO_XFERS
  OCPI::OS::int32_t *src1 = (OCPI::OS::int32_t *)transfer->src_va;
  printf("^^^^ copying %d bytes from %llx to %llx\n", transfer->nbytes,transfer->src_off,transfer->dst_off);
  printf("source wrd 1 = %d wrd2 = %d\n", src1[0], src1[1] );
#endif

}
#else

static void
action_pio_transfer(PIO_transfer transfer)
{
  OCPI::OS::int32_t nwords = ((transfer->nbytes + 5) / 8) ;

#ifdef DEBUG
  {
        int M=50;
        char* c=(char*)src;
        printf("\n\nSource Data(0x%x) Target(0x%x):\n", src, dst );
        for (int n=0; n<M; n++ ) {
                printf("%d ", c[n]);
        }
  }
#endif



#ifdef IP_DEBUG_SUPPORT
  if ( nwords == 1 ) {
    OCPI::OS::int32_t *src1 = (OCPI::OS::int32_t *)transfer->src_va;
    OCPI::OS::int32_t *dst1 = (OCPI::OS::int32_t *)transfer->dst_va;    
    //    if ( ((src1[0] < 3) && (src1[1] < 3)) || ((src1[0] > 2048) && (src1[1] > 2048)) )  {
    if ( ((src1[0] < 3) && (src1[1] < 3)) || ((src1[0] > 2048) && (src1[1] > 2048)) )  {

      /*
      if ( (src1[0] == 1 ) || (src1[1] == 1 ) ) {
        printf("patching flag 1->0 \n");
        dst[0] = 1;
      }
      else {
        dst1[0] = 0;
      }
      */
      
      dst1[0] = 1;
      //      dst1[1] = 1;
    }
    else {
      dst1[0] = src1[0];
    }
  }
  else {
    OCPI::OS::int32_t *src1 = (OCPI::OS::int32_t *)transfer->src_va;
    OCPI::OS::int32_t *dst1 = (OCPI::OS::int32_t *)transfer->dst_va;    
    for (OCPI::OS::int32_t i=0; i < nwords*2; i++) {
      dst1[i] = src1[i];
    }
  }
#endif



  OCPI::OS::int32_t *src1 = (OCPI::OS::int32_t *)transfer->src_va;
  OCPI::OS::int32_t *dst1 = (OCPI::OS::int32_t *)transfer->dst_va;    

  //#define TRACE_PIO_XFERS  
#ifdef TRACE_PIO_XFERS
  printf("^^^^ copying %d bytes from %llx to %llx\n", transfer->nbytes,transfer->src_off,transfer->dst_off);
  printf("source wrd 1 = %d, %d\n", src1[0], src1[1] );
  printf("dst va = %p\n", dst1 );
#endif


  for (OCPI::OS::int32_t i=0; i < nwords*2; i++) {
    dst1[i] = src1[i];
  }
  
  
  
}
#endif


static OCPI::OS::int32_t
is_contiguous(Shape *shape)
{
  if ((shape->x.stride == 1) &&
      (shape->x.length == shape->y.stride) &&
      (shape->y.length * shape->y.stride == shape->z.stride)) {
    return 1;
  }
  else {
    return 0;
  }
}

static OCPI::OS::uint32_t
calc_shape_size(Shape *s)
{
  return (s->x.length * s->y.length * s->z.length * s->element_size);
}

static OCPI::OS::int32_t
process_shapes(Shape *src, Shape *dst, PIO_template pio_template,
               OCPI::OS::uint32_t src_os, OCPI::OS::uint32_t dst_os, PIO_transfer *xf_transfers)
{
  /* Get the shape sizes */
  OCPI::OS::uint32_t src_shape_size = calc_shape_size(src);
  OCPI::OS::uint32_t dst_shape_size = calc_shape_size(dst);

  /* Check the sizes are equal */
  if (src_shape_size != dst_shape_size)
    return 1;

  /* Check to see if shape is contiguous */
  if (is_contiguous(src)) {
    if (is_contiguous(dst)) {
      /* Create a transfer */
      PIO_transfer pio_transfer = new struct pio_transfer_;
      /* Populate the transfer structure */
      pio_transfer->src_va = ((char *)pio_template->src_va + src_os);
      pio_transfer->src_stride = 1 * src->element_size;
      pio_transfer->dst_va = ((char *)pio_template->dst_va + dst_os);
      pio_transfer->dst_stride = 1 * src->element_size;
      pio_transfer->nbytes = src_shape_size;
      /* Set the transfers next pointer */
      pio_transfer->next = 0;
      /* Set the out parameter */
      *xf_transfers = pio_transfer;
    }
  }

  return 0;
}

OCPI::OS::int32_t
xfer_pio_create(DataTransfer::SmemServices* src, DataTransfer::SmemServices* dst, PIO_template *pio_templatep)
{
  void *src_va;
  void *dst_va;

  // Map source and destination
  src_va = src->map (0, 0);
  dst_va = dst->map (0, 0);

  /* Allocate template */
  PIO_template pio_template = new struct pio_template_;

  /* Insert addresses to template */
  pio_template->src_va = src_va;
  pio_template->dst_va = dst_va;
  pio_template->s_smem = src;
  pio_template->t_smem = dst;

  /* Set the out parameter */
  *pio_templatep = pio_template;

  return 0;
}

#define OF_WINDOW 0x1000000

void
xfer_pio_modify( PIO_transfer transfer, int,  OCPI::OS::uint32_t *noff,  OCPI::OS::uint32_t *ooff )
{
  ooff[0] = transfer->src_off;
  transfer->src_va = (char*)transfer->src_va - transfer->src_off;
  transfer->src_off = *noff;
  transfer->src_va = (char*)transfer->src_va + *noff;
}



OCPI::OS::int32_t
xfer_pio_copy(PIO_template pio_template, OCPI::OS::uint32_t src_os, OCPI::OS::uint32_t dst_os,
              OCPI::OS::int32_t nbytes, OCPI::OS::int32_t flags, PIO_transfer *pio_transferp)
{
  /* Allocate transfer */
  PIO_transfer pio_transfer = new struct pio_transfer_;

  /* Check to see if this is a flag transfer */
  if (flags & XFER_FLAG) {
    pio_transfer->src_va = 0;
    pio_transfer->src_stride = 1;
    pio_transfer->dst_va = ((char *)pio_template->src_va + src_os);
    pio_transfer->dst_stride = 1;
    pio_transfer->nbytes = nbytes;
    pio_transfer->next = 0;
  }
  else {
    /* Populate transfer */
    pio_transfer->src_va = ((char *)pio_template->src_va + src_os);
    pio_transfer->src_off = src_os;
    pio_transfer->dst_off = dst_os;
    pio_transfer->src_stride = 1;
        
#define MULTI_MAP
#ifdef MULTI_MAP
    if ( dst_os >= OF_WINDOW ) {
                
#ifndef NDEBUG
      printf("*** Got a window bigger than original map, offset = 0x%x !!\n", dst_os );
#endif
                
      pio_template->rdst_va[pio_template->rdst] = pio_template->t_smem->map (dst_os, nbytes);
      pio_transfer->dst_va = (char*)pio_template->rdst_va[pio_template->rdst];
      pio_template->rdst++;
    }
    else {
      pio_transfer->dst_va = ((char *)pio_template->dst_va + dst_os);
    }
#else
    pio_transfer->dst_va = ((char *)pio_template->dst_va + dst_os);
#endif
                
    pio_transfer->dst_stride = 1;
    pio_transfer->nbytes = nbytes;
    pio_transfer->next = 0;
  }

  /* Set the output parameter */
  *pio_transferp = pio_transfer;

  /* Return success */
  return 0;
}

OCPI::OS::int32_t
xfer_pio_copy_2d(PIO_template pio_template, OCPI::OS::uint32_t src_os, Shape *src,
                 OCPI::OS::uint32_t dst_os, Shape *dst, OCPI::OS::int32_t, 
                 PIO_transfer *pio_transferp)
{

  /* Process the shapes */
  if (process_shapes(src, dst, pio_template, 
                     src_os, dst_os, pio_transferp))
    return 1;

  return 0;
}

OCPI::OS::int32_t
xfer_pio_group(PIO_transfer *members, OCPI::OS::int32_t, PIO_transfer *pio_transferp)
{
  /* Check for null member array */
  if (!members)
    return 1;

  PIO_transfer transfer = new struct pio_transfer_;
  OCPI::OS::int32_t index = 0;

  /* Check the allocation worked */
  if (!transfer)
    return 1;

  /* Check the list is not empty */
  if (!(members[0]))
    return 1;

  /* Copy the first member into transfer */
  *transfer = *(members[0]);

  /* Local Variables */
  PIO_transfer xf = transfer;

  if (members[index]) {
    do {
      /* Local Variables */
      PIO_transfer tmp;

      /* Walk to the end of the current transfer */
      while (xf->next) {
        /* Allocate a new space to copy the transfer into */
        tmp = new struct pio_transfer_;
        /* Copy the next transfer */
        *tmp = *(xf->next);
        /* Set the correct next pointer */
        xf->next = tmp;
        /* Point xf to the new transfer */
        xf = tmp;
      }

      /* Increment the index, and check for end of list */
      if (!(members[++index]))
        break;

      /* Allocate a new member */
      tmp = new struct pio_transfer_;

      /* Copy the member into the new space */
      *tmp = *(members[index]);

      /* Append the new transfer */
      xf->next = tmp;

      /* Point xf to the new transfer */
      xf = tmp;

    } while (members[index]);
  }

  /* Set the out parameter */
  *pio_transferp = transfer;

  /* Return success */
  return 0;
}

OCPI::OS::int32_t
xfer_pio_start(PIO_transfer pio_transfer, OCPI::OS::int32_t)
{
  PIO_transfer transfer = pio_transfer;

  do {
    /* Perform the described transfer */
    action_pio_transfer(transfer);

  } while ((transfer = transfer->next));

  return 0;
}

OCPI::OS::int32_t
xfer_pio_release(PIO_transfer pio_transfer)
{
  /* Local Variables */
  PIO_transfer xf = pio_transfer;

  /* Free the transfer list */
  do {
    /* Local Variables */
    PIO_transfer tmp;
    /* Get the next transfer (if any) */
    tmp = xf->next;
    /* Free the current transfer */
    delete xf;
    /* Copy the next transfer to xf */
    xf = tmp;

  } while (xf);

  return 0;
}

OCPI::OS::int32_t
xfer_pio_destroy(PIO_template pio_template)
{
  /* Free the transfer template */
  delete pio_template;
  return 0;
}
