
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


/*
 * Abstact:
 *   This file contains the Interface for the OCPI data distribution base class.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#ifndef OCPI_DataTransport_DataDistribution
#define OCPI_DataTransport_DataDistribution

#include <OcpiIntDataDistribution.h>
#include <OcpiTransferTemplate.h>

namespace OCPI {

  namespace DataTransport {


    /**********************************
     * This is the data distribution interface whose
     * implementation is responsible for performing the
     * setup and perhaps the actual distribution of the
     * data.
     **********************************/
    class OcpiDataDistribution : public DataDistribution
    {

    public:

      /**********************************
       *  Constructors
       **********************************/
      OcpiDataDistribution( DataDistributionMetaData* meta_data,  Circuit* circuit );


      /**********************************
       *  Destructor
       **********************************/
      virtual ~OcpiDataDistribution();


      /**********************************
       *  Inititialize any transfer data that is required by the 
       *  implementation.
       **********************************/
      virtual void initTransfers();


      /**********************************
       * Creates or retreives an existing transfer handle. Based upon our rank and
       * the distribution type, this template will be created with the proper offset(s) into
       * the source, offsets into the target(s) and appropriate control structures.
       **********************************/
      TransferTemplate* getTxTemplate( OCPI::DataTransport::Buffer* src );

    private:

      // Initialize the source buffers
      void initSourceBuffers();

    };


    /**********************************
     *  inline methods
     **********************************/


  }

}


#endif


