
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
 *   This file contains the Interface for the OCPI buffer class.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#ifndef OCPI_DataTransport_Buffer_H_
#define OCPI_DataTransport_Buffer_H_

#include <OcpiOsDataTypes.h>
#include <OcpiList.h>
#include <OcpiTransportConstants.h>
#include <DtHandshakeControl.h>
#include <OcpiOsMutex.h>


namespace OCPI {

  namespace DataTransport {

    class  Port;

    class Buffer
    {

    public:


      /**********************************
       * Constructors
       *********************************/
      Buffer( OCPI::DataTransport::Port* port, OCPI::OS::uint32_t tid );

      /**********************************
       * Destructor
       *********************************/
      virtual ~Buffer();

      /**********************************
       * This is normally used when an externally local user is either filling or receiving buffer data.
       *********************************/
      inline void setNoTransfer( bool tx ){m_noTransfer=tx;}

      /**********************************
       * Update offsets
       *********************************/
      virtual void update(bool critical)=0;

      /**********************************
       * Is this buffer empty
       *********************************/
      virtual bool isEmpty()=0;

      /**********************************
       * Get/Set the number of bytes transfered
       *********************************/
      virtual OCPI::OS::uint32_t getNumberOfBytesTransfered()=0;
      virtual void setNumberOfBytes2Transfer(OCPI::OS::uint32_t length)=0;

      /**********************************
       * Marks buffer as full
       *********************************/
      virtual void markBufferFull()=0;

      /**********************************
       * Marks buffer as full
       *********************************/
      virtual void markBufferEmpty()=0;

      /**********************************
       * Get this buffers local state structure
       **********************************/              
      virtual volatile DataTransfer::BufferState* getState()=0; 

      /**********************************
       * Get the offset to this ports meta-data
       **********************************/
      virtual volatile DataTransfer::BufferMetaData* getMetaData()=0;
      volatile DataTransfer::BufferMetaData* getMetaDataByIndex(OCPI::OS::uint32_t idx);
      volatile DataTransfer::BufferMetaData* getMetaDataByPortId( OCPI::OS::uint32_t id );

      /**********************************
       * Get the internal buffer 
       **********************************/
      virtual volatile void* getBuffer();

      /**********************************
       * Send the buffer
       **********************************/
      void send();

      /**********************************
       * Buffer in use
       **********************************/
      bool inUse();
      void setInUse( bool in_use );

      /**********************************
       * DEBUG: print offsets
       **********************************/
      void dumpOffsets();

      /**********************************
       * Sets the buffers busy factor
       **********************************/
      virtual void setBusyFactor( OCPI::OS::uint32_t){};

      /**********************************
       * List of pending transfers associated with this buffer
       **********************************/
      List& getPendingTxList();
      OCPI::OS::uint32_t getPendingTransferCount();

        
      /**********************************
       * Marks/Unmarks the attached buffers that are using
       * this buffer for zero copy
       **********************************/ 
      void markZeroCopyDependent( Buffer* dependent_buffer );
      void unMarkZeroCopyDependent( Buffer* dependent_buffer );


      /**********************************
       * Attach a zero copy buffer to this buffer
       **********************************/
      virtual void attachZeroCopy( Buffer* from_buffer );

      /**********************************
       * Attach a zero copy buffer to this buffer
       **********************************/
      virtual Buffer* detachZeroCopy();

      /**********************************
       * Get the buffers length
       *********************************/
       OCPI::OS::uint32_t getLength();

      /**********************************
       * Get the parent port/port set
       *********************************/
      Port*    getPort();

      /**********************************
       * Get this buffers temporal id
       *********************************/
      OCPI::OS::int32_t getTid();

      // If we are in a Zero Copy state, this is our output buffer
      Buffer* m_zeroCopyFromBuffer;


      // Buffer Address
      volatile void* m_baseAddress;

      // offset from base address to start of this buffer
      DtOsDataTypes::Offset m_startOffset;

      // Buffer length
      OCPI::OS::int32_t m_length;

      // Our port
      OCPI::DataTransport::Port*          m_port;

      // This member is used when a buffer from one port is transfered thru another port.
      // This is used to allow a zero copy transfer from one port to another.
      OCPI::DataTransport::Port*    m_zCopyPort;
      OCPI::DataTransport::Buffer*  m_attachedZBuffer;

      // Opaque pointer for our control class
      void* opaque;

      // temporal id
      OCPI::OS::int32_t m_tid;

      // Indicates if there is already a pull transfer in process
      Buffer* m_pullTransferInProgress;

    protected:


      OCPI::OS::int32_t  m_pid;

      // Mapped pointer to our meta data
      volatile DataTransfer::BufferMetaData  (*m_sbMd)[MAX_PCONTRIBS];
      void*                     m_bmdVaddr;        // buffer meta data virtual address

      // Mapped pointer to our state
      volatile DataTransfer::BufferState  (*m_state)[MAX_PCONTRIBS];
      void*                  m_bsVaddr;                // buffer state virtual address

      // Mapped pointer to our buffer
      volatile void     *m_buffer;
      void*              m_bVaddr;                // buffer virtual address

      // If we are zero copy, the buffer that we are attached to goes here
      OCPI::Util::VList m_dependentZeroCopyPorts;
      unsigned int m_dependentZeroCopyCount;

      // Last mcos error
      OCPI::OS::int32_t m_mcosReturnCode;

      // buffer in use
      bool m_InUse;

      // List of pending transfers attached to this buffer
      List m_pendingTransfers;

      // Remote zero copy buffers attached
      bool m_remoteZCopy;

      // Our thread safe mutex
      OCPI::OS::Mutex m_threadSafeMutex;

      // Do not actually transfer the data
      bool m_noTransfer;


    };


    /**********************************
     ****
     * inline declarations
     ****
     *********************************/


    /**********************************
     * Get the meta-data by index
     **********************************/
    inline volatile DataTransfer::BufferMetaData* Buffer::getMetaDataByIndex(OCPI::OS::uint32_t idx)
      {return &m_sbMd[0][idx];}


    /**********************************
     * Buffer in use
     **********************************/
    inline bool Buffer::inUse()
      {
        return m_InUse;
      }
    inline void Buffer::setInUse( bool in_use )
      {
        m_InUse=in_use;
      }

    /**********************************
     * List of pending transfers associated with this buffer
     **********************************/
    inline List& Buffer::getPendingTxList(){return m_pendingTransfers;}
    inline OCPI::OS::uint32_t Buffer::getPendingTransferCount(){return get_nentries(&m_pendingTransfers);}
    inline Port* Buffer::getPort(){return m_port;}
    inline OCPI::OS::int32_t Buffer::getTid(){return m_tid;}
    inline OCPI::OS::uint32_t Buffer::getLength(){return m_length;}

  }
}


#endif
