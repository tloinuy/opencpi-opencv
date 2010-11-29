
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
 *   This file contains the implementation for the OCPI input buffer class.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#include <OcpiInputBuffer.h>
#include <OcpiPort.h>
#include <OcpiCircuit.h>
#include <OcpiOutputBuffer.h>
#include <OcpiOsAssert.h>
#include <OcpiUtilAutoMutex.h>
#include <OcpiTransferController.h>
#include <OcpiPullDataDriver.h>

using namespace OCPI::DataTransport;
using namespace DataTransfer;
using namespace OCPI::OS;

#define EFLAG_VALUE 5

/**********************************
 * Constructors
 *********************************/
InputBuffer::InputBuffer( OCPI::DataTransport::Port* port, OCPI::OS::uint32_t tid )
  : Buffer( port, tid ),m_feedbackDesc(port->getMetaData()->m_externPortDependencyData),
    m_produced(true)
{
  m_feedbackDesc.type = OCPI::RDT::ProducerDescT;
  m_bVaddr = 0;
  m_bsVaddr = 0;
  m_bmdVaddr = 0;
  memset(m_rssVaddr,0,sizeof(void*)*MAX_PCONTRIBS);

  // update our offsets
  update(0);
}


/**********************************
 * Get/Set the number of bytes transfered
 *********************************/
OCPI::OS::uint32_t InputBuffer::getNumberOfBytesTransfered()
{
  return (OCPI::OS::uint32_t) getMetaDataByIndex(0)->ocpiMetaDataWord.length;
}



/**********************************
 * Update offsets
 *********************************/
void InputBuffer::update(bool critical)
{
  ( void ) critical;
  OCPI::OS::uint32_t n, tid;

#ifndef NDEBUG
  printf("InputBuffer:update:start\n");
#endif

  tid = getTid();

  // We need to know the number of ports in the output port set;
  m_outputPortCount = getPort()->getCircuit()->getOutputPortSet()->getPortCount();
 
  struct PortMetaData::InputPortBufferControlMap *input_offsets = 
    &getPort()->getMetaData()->m_bufferData[tid].inputOffsets;

  // First we will map our buffer
  if ( !this->m_port->isShadow() && !m_bVaddr && input_offsets->bufferOffset ) {

#ifndef NDEBUG
    printf("InputBuffer:update:mapping buffer offset\n");
#endif

    m_bVaddr = getPort()->getLocalShemServices()->map
      (input_offsets->bufferOffset, 
       input_offsets->bufferSize );
    m_startOffset = input_offsets->bufferOffset;
                  
    m_length = input_offsets->bufferSize;
    memset(m_bVaddr, 0, input_offsets->bufferSize);
    m_buffer = m_baseAddress = m_bVaddr;

#ifdef DEBUG_L2
    printf("Input buffer addr = 0x%x, size = %d\n", m_buffer, input_offsets->bufferSize );
#endif

  }

  // map our states
  if ( !this->m_port->isShadow() && !m_bsVaddr && input_offsets->localStateOffset ) {

#ifndef NDEBUG
    printf("InputBuffer:update: mapping states\n");
#endif

    m_bsVaddr = getPort()->getLocalShemServices()->map
      (input_offsets->localStateOffset, 
       sizeof(BufferState)*MAX_PCONTRIBS*2);

    m_state = static_cast<volatile BufferState (*)[MAX_PCONTRIBS]>(m_bsVaddr);
    for ( unsigned int y=0; y<MAX_PCONTRIBS; y++ ) {
      m_state[0][y].bufferFull = EFLAG_VALUE;
    }

    // This separates the flag that we get from the output and the flag that we send to
    // our shadow buffer. 
    for ( unsigned int y=MAX_PCONTRIBS; y<MAX_PCONTRIBS*2; y++ ) {
      m_state[0][y].bufferFull = EFLAG_VALUE;
    }


  }

  // Now map the meta-data
  if ( !this->m_port->isShadow() && !m_bmdVaddr && input_offsets->metaDataOffset ) {

#ifndef NDEBUG
    printf("InputBuffer:update: mapping meta data\n");
#endif

    m_bmdVaddr = getPort()->getLocalShemServices()->map
      (input_offsets->metaDataOffset, 
       sizeof(BufferMetaData)*MAX_PCONTRIBS);

    memset(m_bmdVaddr,0,sizeof(BufferMetaData)*MAX_PCONTRIBS);
    m_sbMd = static_cast<volatile BufferMetaData (*)[MAX_PCONTRIBS]>(m_bmdVaddr);
  }
  

  // These are our shadow buffers remote states
  for ( n=0; n<m_outputPortCount; n++ ) {

    PortSet* s_ps = static_cast<PortSet*>(getPort()->getCircuit()->getOutputPortSet());
    OCPI::DataTransport::Port* shadow_port = static_cast<OCPI::DataTransport::Port*>(s_ps->getPortFromIndex(n));

    // At this point our shadows may not be completely defined, so we may have to delay
    if ( !shadow_port || !shadow_port->getRealShemServices() ) {
      continue;
    }

    int idx = shadow_port->getRealShemServices()->getEndPoint()->mailbox;

    // A shadow for a output may not exist if they are co-located
    if ( !m_rssVaddr[idx] && input_offsets->myShadowsRemoteStateOffsets[idx] ) {

#ifdef DEBUG_L2
      printf("&&&& mapping shadow offset to 0x%x\n", 
             input_offsets->myShadowsRemoteStateOffsets[idx]);
#endif

#ifndef NDEBUG
      printf("InputBuffer:update: mapping shadows\n");
#endif

      m_rssVaddr[idx] = shadow_port->getRealShemServices()->map
        (input_offsets->myShadowsRemoteStateOffsets[idx], 
         sizeof(BufferState));

      // Now format our descriptor
      this->m_feedbackDesc.type = OCPI::RDT::ConsumerFlowControlDescT;
      this->m_feedbackDesc.desc.emptyFlagBaseAddr = 
        input_offsets->myShadowsRemoteStateOffsets[idx];

      // Our flag is 4 bytes
      this->m_feedbackDesc.desc.emptyFlagPitch = sizeof(BufferState);
      this->m_feedbackDesc.desc.emptyFlagSize = sizeof(BufferState);
      this->m_feedbackDesc.desc.emptyFlagValue = 0x1000; 
        
#ifndef NDEBUG                
      printf("Requested Emptyflag port value = 0x%llx\n", 
             (long long)this->m_feedbackDesc.desc.emptyFlagValue);
#endif
                

#ifndef NDEBUG
      printf("InputBuffer:update: map returned %p\n", m_rssVaddr[idx]);
#endif


      m_myShadowsRemoteStates[idx] = 
        static_cast<volatile BufferState*>(m_rssVaddr[idx]);


      m_myShadowsRemoteStates[idx]->bufferFull = EFLAG_VALUE;

#ifdef DEBUG_L2
      printf("Mapped shadow buffer for idx %d = 0x%x\n", idx, m_myShadowsRemoteStates[idx] );
#endif

    }

  }

#ifndef NDEBUG
  printf("InputBuffer:update:finish\n");
#endif

}


void InputBuffer::useTidForFlowControl( bool )
{
  for ( unsigned int y=MAX_PCONTRIBS; y<MAX_PCONTRIBS*2; y++ ) {
    m_state[0][y].bufferFull = EFLAG_VALUE;
  }
}



void InputBuffer::produce( Buffer* src_buf )
{
  if ( m_noTransfer ) return;
  this->getPort()->getPortSet()->getTxController()->produce( src_buf );
  m_produced = true;  
}

/**********************************
 * Sets the buffers busy factor
 **********************************/
void InputBuffer::setBusyFactor( OCPI::OS::uint32_t bf )
{
#ifdef LEAST_BUSY
  m_state[0][m_pid].pad = bf;
#else
  ( void ) bf;
#endif
}


/**********************************
 * Get buffer state
 *********************************/
volatile BufferState* InputBuffer::getState()
{
  if ( !this->getPort()->isShadow() && m_zeroCopyFromBuffer ) {
    return m_zeroCopyFromBuffer->getState();
  }

#ifdef DEBUG_L2
  printf("In input get state, state = 0x%x\n", m_state[0]);
#endif


  if ( ! this->getPort()->isShadow() ) {

#ifdef DEBUG_L2
    printf("Getting load factor of %d\n", m_state[0]->pad );
#endif
    m_tState.bufferFull = m_state[0][m_pid].bufferFull;

#ifdef LEAST_BUSY
    m_tState.pad = m_state[0][m_pid].pad;
#endif


#define NEED_FULL_STATE_CONTROL
#ifdef NEED_FULL_STATE_CONTROL

    // This method belongs in the controller !!!!!


    // If we are a shadow port (a local state of a real remote port), then we only
    // use m_state[0] to determine if the remote buffer is free.  Otherwise, we need
    // to look at all of the other states to determine if all outputs have written to us.
    for ( OCPI::OS::uint32_t n=0; n<MAX_PCONTRIBS; n++ ) {
      if ( m_state[0][n].bufferFull != EFLAG_VALUE ) {
        m_tState.bufferFull = m_state[0][n].bufferFull;
	break;
      }
    }
#endif


  }
  else {

    int mb = getPort()->getMailbox();

    m_tState.bufferFull = m_myShadowsRemoteStates[mb]->bufferFull;

#ifdef LEAST_BUSY
    m_tState.pad = m_myShadowsRemoteStates[mb]->pad;
#endif

#ifdef DEBUG_L2
    printf("Load factor in shadow pad = %d\n",  m_tState.pad );
#endif

  }

  return static_cast<volatile BufferState*>(&m_tState);
}


/**********************************
 * Marks buffer as full
 *********************************/
void InputBuffer::markBufferFull()
{
  m_produced = true;

  if ( ! this->getPort()->isShadow() ) {
    m_state[0][0].bufferFull = 1;
  }
  else {
    m_myShadowsRemoteStates[getPort()->getMailbox()]->bufferFull = 1;
  }

}


/**********************************
 * Marks buffer as empty
 *********************************/
void InputBuffer::markBufferEmpty()
{
  if ( ! this->getPort()->isShadow() ) {
    for ( unsigned int n=0; n<MAX_PCONTRIBS; n++ ) {
      m_state[0][n].bufferFull =  EFLAG_VALUE;
    }
  }
  else {
    m_myShadowsRemoteStates[getPort()->getMailbox()]->bufferFull = EFLAG_VALUE;
    volatile BufferState* state = this->getState();
    state->bufferFull = EFLAG_VALUE;
  }        
}


/**********************************
 * Get buffer state
 *********************************/
volatile BufferState* InputBuffer::getState(OCPI::OS::uint32_t rank)
{
  return &m_state[0][rank];
}


/**********************************
 * Is buffer End Of Whole EOS
 *********************************/
bool InputBuffer::isShadow()
{
  return reinterpret_cast<OCPI::DataTransport::Port*>(getPort())->isShadow();
}



/**********************************
 * Is this buffer empty
 *********************************/
bool InputBuffer::isEmpty()
{

  volatile BufferState* state = this->getState();

  if ( Buffer::isEmpty() == false ) {
    return false;
  }

  if ( getPort()->getPullDriver() ) {
     if ( isShadow() ) {
       // m_produced is our barrier sync.  
       if ( m_produced ) {

         OCPI::OS::uint64_t mdata;
         bool empty = getPort()->getPullDriver()->checkBufferEmpty( (OCPI::OS::uint8_t*)getBuffer(),getLength(),mdata);
         
       //         bool empty = getPort()->getPullDriver()->checkBufferEmpty(this);         
         if ( empty ) {
           m_produced = false;
           return true;
         }
       }       
       return false;
     }
     else {  // We are a real input
       bool empty = 
         (state->bufferFull == EFLAG_VALUE) ? true : false;       
       if ( empty ) {

         OCPI::OS::uint64_t mdata;
         empty = getPort()->getPullDriver()->checkBufferEmpty((OCPI::OS::uint8_t*)getBuffer(),getLength(),mdata);
         if ( !empty ) {
           setInUse(true);
           markBufferFull();
           memcpy((void*)&(getMetaData()->ocpiMetaDataWord), &mdata, sizeof(RplMetaData) );
           setInUse(false);            
         }
       }
       return empty;
     }
   }

#ifdef DEBUG_L2
  printf("Input buffer state = %d\n", m_state[0]->bufferFull);
#endif

#ifdef LEAST_BUSY
  OCPI::DataTransport::Port* sp = static_cast<OCPI::DataTransport::Port*>(this->getPort());
  if ( state->pad != 0) {
    sp->setBusyFactor( state->pad );
  }
#endif

#ifdef DEBUG_L2
  printf("Shadow(%d), Buffer state = %x\n", (isShadow() == true) ? 1:0, state->bufferFull );
#endif

  bool empty;
  empty = (state->bufferFull == EFLAG_VALUE) ? true : false;


#ifdef DEBUG_L2
  printf("TB(%p) port = %p empty = %d\n", this, getPort(), empty );
#endif

  return empty;
}




/**********************************
 * Get the offset to this ports meta-data
 **********************************/
volatile BufferMetaData* InputBuffer::getMetaData()
{
  if ( m_zeroCopyFromBuffer ) {
    return m_zeroCopyFromBuffer->getMetaData();
  }

  // This returns the first MD that has been written
  PortSet* s_port = static_cast<OCPI::DataTransport::PortSet*>(getPort()->getCircuit()->getOutputPortSet());
  for ( OCPI::OS::uint32_t n=0; n<m_outputPortCount; n++ ) {
    int id = s_port->getPortFromIndex(n)->getPortId();
    if ( m_state[0][id].bufferFull  != EFLAG_VALUE ) {
      return &m_sbMd[0][id];
    }
  }
  return &m_sbMd[0][m_pid];

}


/**********************************
 * Get number of outputs that have written to this buffer
 *********************************/
OCPI::OS::uint32_t InputBuffer::getNumOutputsThatHaveProduced()
{
  int count=0;
  PortSet* s_port = static_cast<OCPI::DataTransport::PortSet*>(getPort()->getCircuit()->getOutputPortSet());
  for ( OCPI::OS::uint32_t n=0; n<m_outputPortCount; n++ ) {
    int id = s_port->getPortFromIndex(n)->getPortId();
    if ( m_state[0][id].bufferFull
 != EFLAG_VALUE  ) {
      count++;
    }
  }
  return count;
}



/**********************************
 * Get the Output produced meta data by index
 *********************************/
volatile BufferMetaData* InputBuffer::getMetaDataByIndex( OCPI::OS::uint32_t idx )
{
  PortSet* s_port = static_cast<OCPI::DataTransport::PortSet*>(getPort()->getCircuit()->getOutputPortSet());
  int id = s_port->getPortFromIndex(idx)->getPortId();
  return &m_sbMd[0][id];
}




      
/**********************************
 * Destructor
 *********************************/
InputBuffer::~InputBuffer()
{


}


