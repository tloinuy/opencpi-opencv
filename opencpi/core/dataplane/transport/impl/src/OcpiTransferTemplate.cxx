
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
 *   This file contains the implementation for the OCPI transfer template class.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#include <DtHandshakeControl.h>
#include <OcpiTransferTemplate.h>
#include <OcpiPortSet.h>
#include <OcpiOsAssert.h>

using namespace OCPI::DataTransport;
using namespace DataTransfer;


/**********************************
 * init 
 *********************************/
void 
OcpiTransferTemplate::
init()
{
  n_transfers = 0;
  m_zCopy = NULL;
  m_maxSequence = 0;
}


/**********************************
 * Constructors
 *********************************/
OcpiTransferTemplate::
OcpiTransferTemplate(OCPI::OS::uint32_t id)
  :m_id(id)
{
  // Perform initialization
  init();
}


/**********************************
 * Destructor
 *********************************/
OcpiTransferTemplate::
~OcpiTransferTemplate()
{
  if ( m_zCopy )
    delete m_zCopy;

  for ( OCPI::OS::uint32_t n=0; n<n_transfers; n++ ) {
    delete m_xferReq[n];
  }
}


/**********************************
 * Make sure we dont already have this pair
 *********************************/
bool 
OcpiTransferTemplate::
isDuplicate(  OutputBuffer* output, InputBuffer* input )
{
  if ( m_zCopy ) {
    ZCopy* z = m_zCopy;
    while( z ) {
      if ( (z->output == output ) && ( z->input == input) ) {
        return true;
      }
      z = z->next;
    }
  }
  return false;
}


/**********************************
 * Add a xero copy transfer request
 *********************************/
void 
OcpiTransferTemplate::
addZeroCopyTransfer( 
                    OutputBuffer* output, 
                    InputBuffer* input )
{

#ifndef NDEBUG
  printf("addZeroCopyTransfer, adding Zero copy transfer, output = %p, input = %p\n", output, input);
#endif

  if ( isDuplicate( output, input) ) {
    return;
  }

  ZCopy * zc = new ZCopy(output,input);
  if ( ! m_zCopy ) {
    m_zCopy = zc;
  }
  else {
    m_zCopy->add( zc );
  }

}


/**********************************
 * Is this transfer complete
 *********************************/
bool 
OcpiTransferTemplate::
isComplete()
{
  // If we have any gated transfers pending, we are not done
  if ( m_sequence < m_maxSequence ) {
    return false;
  }

  // Make sure all of our transfers are complete
  for ( OCPI::OS::uint32_t n=0; n<n_transfers; n++ ) {
    if ( m_xferReq[n]->getStatus() != 0 ) {
      return false;
    }
  }

  // now make sure all of the gated transfers are complete
  OCPI::OS::int32_t n_pending = get_nentries(&m_gatedTransfersPending);

#ifndef NDEBUG
  printf("There are %d gated transfers\n", n_pending );
#endif

  for (OCPI::OS::int32_t i=0; i < n_pending; i++) {
    OcpiTransferTemplate* temp = static_cast<OcpiTransferTemplate*>(get_entry(&m_gatedTransfersPending, i));
    if ( temp->isComplete() ) {
      remove_from_list( &m_gatedTransfersPending, temp );
      n_pending = get_nentries(&m_gatedTransfersPending);
      i = 0;
    }
    else {
      return false;
    }
  }

  return true;
}

OCPI::OS::uint32_t 
OcpiTransferTemplate::
produceGated( OCPI::OS::uint32_t port_id, OCPI::OS::uint32_t tid  )
{

  // See if we have any other transfers of this type left
#ifndef NDEBUG
  printf("m_sequence = %d\n", m_sequence );
#endif

  OcpiTransferTemplate* temp = getNextGatedTransfer(port_id,tid);

  if ( temp ) {
    temp->produce();
  }
  else {

#ifndef NDEBUG
    printf("ERROR !!! OcpiTransferTemplate::produceGated got a NULL template, p = %d, t = %d\n",
           port_id, tid);
#endif

    ocpiAssert(0);
  }

  // Add the template to our list
  insert_to_list(&m_gatedTransfersPending, temp, 64, 8);
        
  return m_maxSequence - m_sequence;

}



/**********************************
 * Presets values into the output meta-data prior to kicking off
 * a transfer
 *********************************/
void 
OcpiTransferTemplate::
presetMetaData( 
               volatile BufferMetaData* data, 
               OCPI::OS::uint32_t length,
               bool end_of_whole,
               OCPI::OS::uint32_t parts_per_whole,                
               OCPI::OS::uint32_t sequence)                                
{

  OCPI::OS::uint32_t seq_inc=0;

  PresetMetaData *pmd = new PresetMetaData();
  pmd->ptr = data;
  pmd->length = length;
  pmd->endOfWhole = end_of_whole ? 1 : 0;
  pmd->nPartsPerWhole = parts_per_whole;
  pmd->sequence = sequence + seq_inc;
  seq_inc++;

  // Add the structure to our list
  insert_to_list(&m_PresetMetaData, pmd, 64, 8);

}


/**********************************
 * Presets values into the output meta-data prior to kicking off
 * a transfer
 *********************************/
void 
OcpiTransferTemplate::
presetMetaData()
{
  OCPI::OS::int32_t n_pending = get_nentries(&m_PresetMetaData);
  for (OCPI::OS::int32_t i=0; i < n_pending; i++) {
    PresetMetaData *pmd = static_cast<PresetMetaData*>(get_entry(&m_PresetMetaData, i));
    pmd->ptr->ocpiMetaDataWord.length = pmd->length;
    pmd->ptr->endOfWhole         = pmd->endOfWhole;
    pmd->ptr->nPartsPerWhole     = pmd->nPartsPerWhole;
    pmd->ptr->partsSequence      = pmd->sequence;
  }
           
}





/**********************************
 * start the transfer
 *********************************/
void 
OcpiTransferTemplate::
produce()
{
  // start the transfers now
#ifndef NDEBUG
  printf("Producing %d transfers\n", n_transfers);
#endif

  // At this point we need to pre-set the meta-data if needed
  presetMetaData();

  // If we have a list of attached buffers, it means that we need to 
  // inform the local buffers that there is zero copy data available
  if ( m_zCopy ) {
    ZCopy* z = m_zCopy;
    while( z ) {
#ifndef NDEBUG
      printf("Attaching %p to %p\n", z->output, z->input);
#endif
      z->input->attachZeroCopy( z->output );
      z = z->next;
    }
  }

  // Remote transfers
  for ( OCPI::OS::uint32_t n=0; n<n_transfers; n++ ) {
    m_xferReq[n]->start();
  }

  // Now increment our gated transfer control
  m_sequence = 0;

}



/**********************************
 * start the transfer
 *********************************/
Buffer* 
OcpiTransferTemplate::
consume()
{
  Buffer* rb=NULL;

#ifndef NDEBUG
  printf("Consuming %d transfers\n", n_transfers);
#endif

  // If we have a list of attached buffers, it means that we are now
  // done with the local zero copy buffers and need to mark them 
  // as available
  if ( m_zCopy ) {
    ZCopy* z = m_zCopy;
    while( z ) {
#ifndef NDEBUG
      printf("Detaching %p to %p\n", z->output, z->input);
#endif
      rb = z->input->detachZeroCopy();
      z = z->next;
    }
  }

  // Remote transfers
  for ( OCPI::OS::uint32_t n=0; n<n_transfers; n++ ) {
    m_xferReq[n]->start();
  }

  return rb;

}

/**********************************
 * Start the input reply transfer
 *********************************/
void OcpiTransferTemplate::modify( OCPI::OS::uint32_t new_off[], OCPI::OS::uint32_t old_off[] )
{
  for ( OCPI::OS::uint32_t n=0; n<n_transfers; n++ ) {
    m_xferReq[n]->modify( new_off, old_off );
  }
}

/**********************************
 * Is this transfer pending
 *********************************/
bool OcpiTransferTemplate::isPending()
{
  return false;
}


void OcpiTransferTemplate::ZCopy::add( ZCopy * zc )
{
  ZCopy* t = this;
  while ( t->next ) {
    t = t->next;
  }
  t->next = zc;
}

