
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




#ifndef INCLUDED_OCPI_XM_FORTRAN_BUFFERS_H
#define INCLUDED_OCPI_XM_FORTRAN_BUFFERS_H

#ifdef __cplusplus
extern "C" {
#endif

/* ---- OpenOCPI for X-Midas types ---------------------------------------- */

#include <stdint.h>

#ifndef INCLUDED_FORTRAN_H
#define INCLUDED_FORTRAN_H
#define LPROTOTYPE
#include <fortran.h>
#endif

/* ---- OpenOCPI for X-Midas buffer replacement --------------------------- */

typedef struct
{
  union
  {
    char bbuf [ 32768 ];
    char abuf [ 32768 ];
    short ibuf [ 16384 ];
    int lbuf [ 8192 ];
    float fbuf [ 8192 ];
    double dbuf [ 4096 ];
    complex cfbuf [ 4096 ];
    dcomplex cdbuf [ 2048 ];
    char sbuf [ 32768 ];
  };
} Sbuffer;

/* ---- OpenOCPI for X-Midas Sm_ptr replacement --------------------------- */

typedef struct
{
  int p;
  int fill;
} Sm_ptr;

#ifdef __cplusplus
}
#endif

#endif /* End: #ifndef INCLUDED_OCPI_XM_FORTRAN_BUFFERS_H */
