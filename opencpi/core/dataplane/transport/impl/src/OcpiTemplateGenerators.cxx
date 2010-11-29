
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
 *   This file contains the OCPI template generator implementation.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 */

#include <DtTransferInternal.h>
#include <OcpiPortSet.h>
#include <OcpiBuffer.h>
#include <OcpiOutputBuffer.h>
#include <OcpiInputBuffer.h>
#include <OcpiTemplateGenerators.h>
#include <OcpiTransferController.h>
#include <OcpiIntDataDistribution.h>
#include <OcpiOsAssert.h>

#define FORMAT_TRANSFER_EC_RETHROW( sep, tep )                                \
  char buf[512];                                                        \
  strcpy(buf, tep->getEndpoint()->end_point.c_str());                        \
  strcat(buf, " -> ");                                                        \
  strcat( buf, sep->getEndpoint()->end_point.c_str() );                        \
  throw OCPI::Util::EmbeddedException ( UNABLE_TO_CREATE_TX_REQUEST, buf );


using namespace OCPI::DataTransport;
using namespace DataTransfer;
using namespace DtI;
using namespace OCPI::OS;


TransferTemplateGenerator::
TransferTemplateGenerator()
  :m_zcopyEnabled(true)
{
  // Empty
}

TransferTemplateGenerator::
~TransferTemplateGenerator()
{
  // Empty
}


static 
XferRequest* 
Group( XferServices* temp, XferRequest* xfr1, XferRequest* t2 )
{

  /* Group with the existing transfer */
  XferRequest* groups[3];
  XferRequest* ptmp;

  /* Build the list of transfers */
  groups[0] = xfr1;
  groups[1] = t2;
  groups[2] = 0;

  /* Group the transfers */
  ptmp = temp->group (groups);

  /* Release the previous transfer */
  if ( ptmp != t2 )
    delete t2;

  /* Release the just grouped transfer */
  if ( ptmp != xfr1 ) 
    delete xfr1;

  /* Copy the transfer */
  return ptmp;
}


// Call appropriate creator
void  
TransferTemplateGenerator::
create( Transport* transport, PortSet* output, PortSet* input, TransferController* cont )
{
  bool generated=false;

  // We need to create transfers for every output and destination port 
  // the exists in the context of this container.  
  for ( OCPI::OS::uint32_t s_n=0; s_n<output->getPortCount(); s_n++ ) {

    Port* s_port = output->getPort(s_n);

#ifndef NDEBUG
    printf("s port endpoints = %s, %s, %s\n", 
           s_port->getRealShemServices()->getEndPoint()->end_point.c_str(),
           s_port->getShadowShemServices()->getEndPoint()->end_point.c_str(),           
           s_port->getLocalShemServices()->getEndPoint()->end_point.c_str() );
#endif


      // Create a DD specific transfer template
      createOutputTransfers(s_port,input,cont);

      // Create a broadcast template for pair
      createOutputBroadcastTemplates(s_port,input,cont);

    generated = true;

  }

  for ( OCPI::OS::uint32_t t_n=0; t_n<input->getPortCount(); t_n++ ) {

    Port* t_port = input->getPort(t_n);

#ifndef NDEBUG
    printf("t port endpoints = %s, %s, %s\n", 
           t_port->getRealShemServices()->getEndPoint()->end_point.c_str(),
           t_port->getShadowShemServices()->getEndPoint()->end_point.c_str(),           
           t_port->getLocalShemServices()->getEndPoint()->end_point.c_str() );
#endif


    // If the output port is not local, but the transfer role requires us to move data, we need to create transfers
    // for the remote port
    if ( ! transport->isLocalEndpoint( t_port->getRealShemServices()->getEndPoint()->end_point.c_str()  ) ) {
      break;
    }

    // Create the input transfers
    createInputTransfers(output,t_port,cont);

    // Create the inputs broadcast transfers
    createInputBroadcastTemplates(output,t_port,cont);
    
    generated = true;

  }

  if ( ! generated ) {
    ocpiAssert( ! "PROGRAMMING ERROR!! no templates were generated for this circuit !!\n");
  }

}

// These methods are hooks to add additional transfers
XferRequest* TransferTemplateGenerator::addTransferPreData( TDataInterface& tdi )
{
  ( void ) tdi;
  return NULL;
}


XferRequest* TransferTemplateGenerator::addTransferPostData( TDataInterface& tdi)
{
  ( void ) tdi;
  return NULL;
}

XferRequest* TransferTemplateGenerator::addTransferPreState( TDataInterface& tdi)                                                                                        
{
  ( void ) tdi;
  return NULL;
}


// Create the input broadcast template for this set
void TransferTemplateGenerator::createInputBroadcastTemplates(PortSet* output, 
                                                               Port* input, 
                                                               TransferController* cont
                                                               )
{
  OCPI::OS::uint32_t n;

  // We need to create the transfers to tell all of our shadow ports that a buffer
  // became available, 
  int n_t_buffers = input->getBufferCount();

  // We need a transfer template to allow a transfer for each input buffer to its
  // associated shadows
  for ( int t_buffers=0; t_buffers<n_t_buffers; t_buffers++ ) {

    // input buffer
    InputBuffer* t_buf = static_cast<InputBuffer*>(input->getBuffer(t_buffers));
    int t_tid = t_buf->getTid();

    // Create a template
    OcpiTransferTemplate* temp = new OcpiTransferTemplate(0);


    //Add the template to the controller
#ifndef NDEBUG
    printf("*&*&* Adding template for tpid = %d, ttid = %u, template = %p\n", 
           input->getPortId(), t_tid, temp);
#endif

    cont->addTemplate( temp, 0, 0, input->getPortId(), t_tid, true, TransferController::INPUT );

    struct PortMetaData::InputPortBufferControlMap *input_offsets = 
      &input->getMetaData()->m_bufferData[t_tid].inputOffsets;

    // We need to setup a transfer for each shadow, they exist in the unique output circuits
    for ( n=0; n<output->getPortCount(); n++ ) {

      // Since the shadows only exist in the circuits with instances of real
      // output ports, the offsets are indexed via the output port ordinals.
      // If the output is co-located with us, no shadow exists.
      Port* s_port = output->getPort(n);
      int s_pid = s_port->getMailbox();

      // We dont need to do anything for co-location
      if ( m_zcopyEnabled && s_port->supportsZeroCopy( input ) ) {
        temp->addZeroCopyTransfer( NULL, t_buf );
      }

      /* Attempt to get or make a transfer template */
      XferServices* ptemplate = XferFactoryManager::getFactoryManager().getService( 
                                                               input->getEndpoint(), 
                                                               s_port->getEndpoint() );
      if ( ! ptemplate ) {
        FORMAT_TRANSFER_EC_RETHROW( input, s_port );
      }

#ifndef NDEBUG
      printf("CreateInputBroadcastTransfers: localStateOffset 0x%llx\n", 
             (long long)input_offsets->localStateOffset);
      printf("CreateInputBroadcastTransfers: RemoteStateOffsets %p\n",
             input_offsets->myShadowsRemoteStateOffsets);
      printf("CreateInputBroadcastTransfers: s_pid %d\n", s_pid);
#endif

      // Create the copy in the template
      XferRequest* ptransfer;
      try {
        ptransfer = 
          ptemplate->copy (
                           input_offsets->localStateOffset,
                           input_offsets->myShadowsRemoteStateOffsets[s_pid],
                           sizeof(BufferState),
                           XferRequest::FirstTransfer, NULL);
      }
      catch ( ... ) {
        FORMAT_TRANSFER_EC_RETHROW( input, s_port );
      }
      
      // Add the transfer to the template
      temp->addTransfer( ptransfer );

    } // end for each input buffer
  } // end for each output port
}



void 
TransferTemplateGenerator::
createOutputBroadcastTemplates( Port* s_port, PortSet* input,
                                                               TransferController* cont )
{
  int n;
  PortSet* output = s_port->getPortSet();
  int n_s_buffers = output->getBufferCount();
  int n_t_buffers = input->getBufferCount();
  int n_t_ports = input->getPortCount();

  // We need a transfer template to allow a transfer from each output buffer to every
  // input buffer for this pattern.
  for ( int s_buffers=0; s_buffers<n_s_buffers; s_buffers++ ) {

    // output buffer
    OutputBuffer* s_buf = static_cast<OutputBuffer*>(s_port->getOutputBuffer(s_buffers));
    int s_tid = s_buf->getTid();
    int t_tid;

    // We need a transfer template to allow a transfer to each input buffer
    for ( int t_buffers=0; t_buffers<n_t_buffers; t_buffers++ ) {

      // Get the input port
      Port* t_port = input->getPort(0);

      // input buffer
      InputBuffer* t_buf = static_cast<InputBuffer*>(t_port->getInputBuffer(t_buffers));
      t_tid = t_buf->getTid();

      // Create a template
      OcpiTransferTemplate* temp = new OcpiTransferTemplate(0);

      // Add the template to the controller, for this pattern the output port
      // and the input ports remains constant

#ifndef NDEBUG
      printf("output port id = %d, buffer id = %d, input id = %d\n", 
             s_port->getPortId(), s_tid, t_tid );
      printf("Template address = %p\n", temp);
#endif

      cont->addTemplate( temp, s_port->getPortId(),
                         s_tid, 0 ,t_tid, true, TransferController::OUTPUT );

      /*
       *  This transfer is used to mark the local input shadow buffer as full
       */

      struct PortMetaData::OutputPortBufferControlMap *output_offsets = 
        &s_port->getMetaData()->m_bufferData[s_tid].outputOffsets;

      // We need to setup a transfer for each input port. 
#ifndef NDEBUG
      printf("Number of input ports = %d\n", n_t_ports);
#endif
      for ( n=0; n<n_t_ports; n++ ) {

        // Get the input port
        Port* t_port = input->getPort(n);
        t_buf = static_cast<InputBuffer*>(t_port->getBuffer(t_buffers));

        struct PortMetaData::InputPortBufferControlMap *input_offsets = 
          &t_port->getMetaData()->m_bufferData[t_tid].inputOffsets;

        // We need to determine if this can be a Zero copy transfer.  If so, 
        // we dont need to create a transfer template
        if ( m_zcopyEnabled && s_port->supportsZeroCopy( t_port ) ) {

#ifndef NDEBUG
          printf("** ZERO COPY TransferTemplateGenerator::createOutputBroadcastTemplates from %p, to %p\n", s_buf, t_buf);
#endif
          temp->addZeroCopyTransfer( s_buf, t_buf );
          continue;
        }

        /* Attempt to get or make a transfer template */
        XferServices* ptemplate = XferFactoryManager::getFactoryManager().getService( 
                                                                 s_port->getEndpoint(), 
                                                                 t_port->getEndpoint());
        if ( ! ptemplate ) {
          FORMAT_TRANSFER_EC_RETHROW( s_port, t_port );
        }

        // Create the transfer that copys the output data to the input data
        XferRequest* grouped_ptransfer;

        try {

          XferRequest* ptransfer = 
            ptemplate->copy (
                             output_offsets->bufferOffset,
                             input_offsets->bufferOffset,
                             output_offsets->bufferSize,
                             XferRequest::FirstTransfer, NULL);

          // Create the transfer that copys the output meta-data to the input meta-data
          XferRequest* ptransfer_2 =
            ptemplate->copy (
                             output_offsets->metaDataOffset + s_port->getPortId() * sizeof(BufferMetaData),
                             input_offsets->metaDataOffset + s_port->getPortId() * sizeof(BufferMetaData),
                             sizeof(OCPI::OS::int64_t),
                             XferRequest::None, ptransfer);

          grouped_ptransfer = Group( ptemplate, ptransfer, ptransfer_2 );

          // Create the transfer that copys the output state to the remote input state
          ptransfer = ptemplate->copy (
                                       output_offsets->localStateOffset + s_port->getPortId() * sizeof(BufferState),
                                       input_offsets->localStateOffset + s_port->getPortId() * sizeof(BufferState),
                                       sizeof(BufferState),
                                       XferRequest::LastTransfer, ptransfer_2);

          grouped_ptransfer = Group(ptemplate, grouped_ptransfer, ptransfer );

        }
        catch ( ... ) {
          FORMAT_TRANSFER_EC_RETHROW( s_port, t_port );
        }


        // Add the transfer 
        temp->addTransfer( grouped_ptransfer );

      } // end for each input buffer


      // And now to all other outputs

      // A output braodcast must also send to all other outputs to update them as well
      // Now we need to pass the output control baton onto the next output port
      XferRequest* p_transfer[2];
      p_transfer[0] = p_transfer[1] = NULL;
      for ( OCPI::OS::uint32_t ns=0; ns<s_port->getPortSet()->getPortCount(); ns++ ) {
        Port* next_sp = 
          static_cast<Port*>(s_port->getPortSet()->getPortFromIndex(ns));
        if ( next_sp == s_port ) {
          continue;
        }

        struct PortMetaData::OutputPortBufferControlMap *next_output_offsets = 
          &next_sp->getMetaData()->m_bufferData[s_tid].outputOffsets;

        /* Attempt to get or make a transfer template */
        XferServices* ptemplate = XferFactoryManager::getFactoryManager().getService( 
                                                                 s_port->getEndpoint(), 
                                                                 next_sp->getEndpoint() );
        if ( ! ptemplate ) {
          FORMAT_TRANSFER_EC_RETHROW( s_port, next_sp );
        }

        // Create the transfer from out output contol state to the next
        try {
          p_transfer[0] = ptemplate->copy (
                                           output_offsets->portSetControlOffset,
                                           next_output_offsets->portSetControlOffset,
                                           sizeof(OutputPortSetControl),
                                           XferRequest::None, NULL);

          if ( p_transfer[1] ) {
            p_transfer[1] = Group( ptemplate, p_transfer[0], p_transfer[1] );
          }
          else {
            p_transfer[1] = p_transfer[0];
          }
        }
        catch( ... ) {
          FORMAT_TRANSFER_EC_RETHROW( s_port, next_sp );
        }
      }

      // Add the transfer 
      if ( p_transfer[1] ) {
        temp->addTransfer( p_transfer[1] );
      }

    } // end for each output buffer

  }  // end for each input port

}



// This base class provides a default pattern for the input buffers which is to 
// braodcast a input buffers availability to all shadows
void TransferTemplateGenerator::createInputTransfers(PortSet* output, Port* input,
                                                      TransferController* cont )

{
  OCPI::OS::uint32_t n;

  // We need to create the transfers to tell all of our shadow ports that a buffer
  // became available, 
  OCPI::OS::uint32_t n_t_buffers = input->getBufferCount();

  // We need a transfer template to allow a transfer for each input buffer to its
  // associated shadows
  for ( OCPI::OS::uint32_t t_buffers=0; t_buffers<n_t_buffers; t_buffers++ ) {

    // input buffer
    InputBuffer* t_buf = input->getInputBuffer(t_buffers);
    int t_tid = t_buf->getTid();

    // Create a template
    OcpiTransferTemplate* temp = new OcpiTransferTemplate(0);

#ifndef NDEBUG
    printf("*&*&* Adding template for tpid = %d, ttid = %d, template = %p\n", 
           input->getPortId(), t_tid, temp);
#endif

    //Add the template to the controller
    cont->addTemplate( temp, 0, 0, input->getPortId(), t_tid, false, TransferController::INPUT );

    struct PortMetaData::InputPortBufferControlMap *input_offsets = 
      &input->getMetaData()->m_bufferData[t_tid].inputOffsets;

    // Since there may be multiple output ports on 1 processs, we need to make sure we dont send
    // more than 1 time
    int sent[MAX_PCONTRIBS];
    memset(sent,0,sizeof(int)*MAX_PCONTRIBS);

    // We need to setup a transfer for each shadow, they exist in the unique output circuits
    for ( n=0; n<output->getPortCount(); n++ ) {

      // Since the shadows only exist in the circuits with instances of real
      // output ports, the offsets are indexed via the output port ordinals.
      // If the output is co-located with us, no shadow exists.
      Port* s_port = output->getPort(n);
      int s_pid = s_port->getRealShemServices()->getEndPoint()->mailbox;

      if ( sent[s_pid] ) {
        continue;
      }

      // If we are creating a template for whole transfers, we do not recognize anything
      // but output port 0
      if ( (s_port->getPortSet()->getDataDistribution()->getMetaData()->distType == DataDistributionMetaData::parallel) &&
           s_port->getRank() != 0 ) {
        continue;
      }

      // Attach zero-copy for co-location
      if ( m_zcopyEnabled && s_port->supportsZeroCopy( input ) ) {
#ifndef NDEBUG
        printf("Adding Zery copy for input response\n");
#endif
        temp->addZeroCopyTransfer( NULL, t_buf );
      }

      sent[s_pid] = 1;

      /* Attempt to get or make a transfer template */
      XferServices* ptemplate = XferFactoryManager::getFactoryManager().getService( 
                                                               input->getEndpoint(), 
                                                               s_port->getEndpoint() );
      if ( ! ptemplate ) {
        FORMAT_TRANSFER_EC_RETHROW( input, s_port );
      }

      XferRequest* ptransfer;
      try {
        // Create the copy in the template
        ptransfer = 
          ptemplate->copy (
                           input_offsets->localStateOffset + (sizeof(BufferState) * MAX_PCONTRIBS) + (sizeof(BufferState)* input->getPortId()),
                           input_offsets->myShadowsRemoteStateOffsets[s_pid],
                           sizeof(BufferState),
                           XferRequest::LastTransfer, NULL);
      }
      catch( ... ) {
        FORMAT_TRANSFER_EC_RETHROW( input, s_port );
      }

      // Add the transfer to the template
      temp->addTransfer( ptransfer );

    } // end for each input buffer

  } // end for each output port

}


// Here is where all of the work is performed
 TransferController* 
   TransferTemplateGenerator::createTemplates( Transport* transport, 
                                                                 PortSet* output, 
                                                                 PortSet* input, TransferController* temp_controller  )
{
  // Do the work
  create(transport, output,input,temp_controller);

  // return the controller
  return temp_controller;        
}




TransferTemplateGeneratorPattern1::
~TransferTemplateGeneratorPattern1()
{
  // Empty
}


// Create transfers for output port for the pattern w[p] -> w[p]
void TransferTemplateGeneratorPattern1::createOutputTransfers( Port* s_port, PortSet* input,
                                                              TransferController* cont )
{

  // Since this is a whole output distribution, only port 0 of the output
  // set gets to do anything.
  if ( s_port->getPortSet()->getDataDistribution()->getMetaData()->distType == DataDistributionMetaData::parallel &&
       s_port->getRank() != 0 ) {
    return;
  }

  int n;
  PortSet* output = s_port->getPortSet();
  int n_s_buffers = output->getBufferCount();
  int n_t_buffers = input->getBufferCount();
  int n_t_ports = input->getPortCount();

  // We need a transfer template to allow a transfer from each output buffer to every
  // input buffer for this pattern.
  for ( int s_buffers=0; s_buffers<n_s_buffers; s_buffers++ ) {

    // output buffer
    OutputBuffer* s_buf = s_port->getOutputBuffer(s_buffers);
    int s_tid = s_buf->getTid();

    // We need a transfer template to allow a transfer to each input buffer
    for ( int t_buffers=0; t_buffers<n_t_buffers; t_buffers++ ) {

      // Get the input port
      Port* t_port = input->getPort(0);

      // input buffer
      InputBuffer* t_buf = t_port->getInputBuffer(t_buffers);
      int t_tid = t_buf->getTid();

      // Create a template
      OcpiTransferTemplate* temp = new OcpiTransferTemplate(1);


      // Add the template to the controller, for this pattern the output port
      // and the input ports remains constant
#ifndef NDEBUG
      printf("output port id = %d, buffer id = %d, input id = %d\n", 
             s_port->getPortId(), s_tid, t_tid);
      printf("Template address = %p\n", temp);
#endif

      cont->addTemplate( temp, s_port->getPortId(),
                         s_tid, 0 ,t_tid, false, TransferController::OUTPUT );

      /*
       *  This transfer is used to mark the local input shadow buffer as full
       */

      // We need to setup a transfer for each input port. 
#ifndef NDEBUG
      printf("Number of input ports = %d\n", n_t_ports);
#endif

      for ( n=0; n<n_t_ports; n++ ) {

        // Get the input port
        Port* t_port = input->getPort(n);
        t_buf = t_port->getInputBuffer(t_buffers);

        struct PortMetaData::OutputPortBufferControlMap *output_offsets = 
          &s_port->getMetaData()->m_bufferData[s_tid].outputOffsets;

        struct PortMetaData::InputPortBufferControlMap *input_offsets = 
          &t_port->getMetaData()->m_bufferData[t_tid].inputOffsets;

        // We need to determine if this can be a Zero copy transfer.  If so, 
        // we dont need to create a transfer template
        if ( m_zcopyEnabled && s_port->supportsZeroCopy( t_port ) ) {

#ifndef NDEBUG
          printf("** ZERO COPY TransferTemplateGeneratorPattern1::createOutputTransfers from %p, to %p\n", s_buf, t_buf);
#endif
          temp->addZeroCopyTransfer( s_buf, t_buf );
          continue;
        }

        /* Attempt to get or make a transfer template */
        XferServices* ptemplate = XferFactoryManager::getFactoryManager().getService( 
                                                                 s_port->getEndpoint(), 
                                                                 t_port->getEndpoint() );
        if ( ! ptemplate ) {
          FORMAT_TRANSFER_EC_RETHROW( s_port, t_port );
        }

        // Create the transfer that copys the output data to the input data
        XferRequest* grouped_ptransfer;

        try {
          XferRequest* ptransfer = 
            ptemplate->copy (
                             output_offsets->bufferOffset,
                             input_offsets->bufferOffset,
                             output_offsets->bufferSize,
                             XferRequest::FirstTransfer, NULL);

          // Create the transfer that copys the output meta-data to the input meta-data
          XferRequest* ptransfer_2 =
            ptemplate->copy (
                             output_offsets->metaDataOffset + s_port->getPortId() * sizeof(BufferMetaData),
                             input_offsets->metaDataOffset + s_port->getPortId() * sizeof(BufferMetaData),
                             sizeof(OCPI::OS::int64_t),
                             XferRequest::DataOffset, ptransfer );

          grouped_ptransfer = Group( ptemplate, ptransfer, ptransfer_2 );

          // Create the transfer that copys the output state to the remote input state
          ptransfer = ptemplate->copy (
                                       output_offsets->localStateOffset + s_port->getPortId() * sizeof(BufferState),
                                       input_offsets->localStateOffset + s_port->getPortId() * sizeof(BufferState),
                                       sizeof(BufferState),
                                       XferRequest::LastTransfer, grouped_ptransfer);

          grouped_ptransfer = Group(ptemplate, grouped_ptransfer, ptransfer );
        }
        catch( ... ) {
          FORMAT_TRANSFER_EC_RETHROW( s_port, t_port );
        }

        // Add the transfer 
        temp->addTransfer( grouped_ptransfer );

      } // end for each input buffer

    } // end for each output buffer

  }  // end for each input port

}


void TransferTemplateGeneratorPattern1::createInputTransfers(PortSet* output, Port* input,
                                                              TransferController* cont )

{

  // For this pattern, we can use the base class implementation which broadcasts a
  // input buffers availability
  TransferTemplateGenerator::createInputTransfers( output, input, cont );
}


class OcpiTransferTemplateAFC : public OcpiTransferTemplate
{
public:
  OcpiTransferTemplateAFC(OCPI::OS::uint32_t id)
    :OcpiTransferTemplate(id){}
  virtual ~OcpiTransferTemplateAFC(){}
  virtual bool isSlave()
  {
    printf("\n\n\n\n I Am A slave port \n\n\n\n\n");
    return true;
  }
};






// Create transfers for a output port that has a ActiveFlowControl role.  This means that the 
// the only transfer that takes place is the "flag" transfer.  It is the responibility of the 
// remote "pull" port to tell us when our output buffer becomes free.
void 
TransferTemplateGeneratorPattern1AFC::
createOutputTransfers(OCPI::DataTransport::Port* s_port, 
                          OCPI::DataTransport::PortSet* input,
                          TransferController* cont )
{

  // Since this is a whole output distribution, only port 0 of the output
  // set gets to do anything.
  if ( s_port->getPortSet()->getDataDistribution()->getMetaData()->distType == DataDistributionMetaData::parallel &&
       s_port->getRank() != 0 ) {
    return;
  }

  int n;
  PortSet* output = s_port->getPortSet();
  int n_s_buffers = output->getBufferCount();
  int n_t_buffers = input->getBufferCount();
  int n_t_ports = input->getPortCount();

  // We need a transfer template to allow a transfer from each output buffer to every
  // input buffer for this pattern.
  for ( int s_buffers=0; s_buffers<n_s_buffers; s_buffers++ ) {

    // output buffer
    OutputBuffer* s_buf = s_port->getOutputBuffer(s_buffers);
    s_buf->setSlave();
    int s_tid = s_buf->getTid();

    // We need a transfer template to allow a transfer to each input buffer
    for ( int t_buffers=0; t_buffers<n_t_buffers; t_buffers++ ) {

      // Get the input port
      Port* t_port = input->getPort(0);

      // input buffer
      InputBuffer* t_buf = t_port->getInputBuffer(t_buffers);
      int t_tid = t_buf->getTid();

      // Create a template
      OcpiTransferTemplate* temp = new OcpiTransferTemplateAFC(1);


      // Add the template to the controller, for this pattern the output port
      // and the input ports remains constant
#ifndef NDEBUG
      printf("output port id = %d, buffer id = %d, input id = %d\n", 
             s_port->getPortId(), s_tid, t_tid);
      printf("Template address = %p\n", temp);
#endif

      cont->addTemplate( temp, s_port->getPortId(),
                         s_tid, 0 ,t_tid, false, TransferController::OUTPUT );

      // We need to setup a transfer for each input port. 
#ifndef NDEBUG
      printf("Number of input ports = %d\n", n_t_ports);
#endif

      for ( n=0; n<n_t_ports; n++ ) {

        // Get the input port
        Port* t_port = input->getPort(n);
        t_buf = t_port->getInputBuffer(t_buffers);

        struct PortMetaData::OutputPortBufferControlMap *output_offsets = 
          &s_port->getMetaData()->m_bufferData[s_tid].outputOffsets;

        struct PortMetaData::InputPortBufferControlMap *input_offsets = 
          &t_port->getMetaData()->m_bufferData[t_tid].inputOffsets;

        // We need to determine if this can be a Zero copy transfer.  If so, 
        // we dont need to create a transfer template
        if ( m_zcopyEnabled && s_port->supportsZeroCopy( t_port ) ) {

#ifndef NDEBUG
          printf("** ZERO COPY TransferTemplateGeneratorPattern1AFC::createOutputTransfers from %p, to %p\n", s_buf, t_buf);
#endif
          temp->addZeroCopyTransfer( s_buf, t_buf );
          continue;
        }

        /* Attempt to get or make a transfer template */
        XferServices* ptemplate = XferFactoryManager::getFactoryManager().getService( 
                                                                 s_port->getEndpoint(), 
                                                                 t_port->getEndpoint() );
        if ( ! ptemplate ) {
          FORMAT_TRANSFER_EC_RETHROW( s_port, t_port );
        }

        // Create the transfer that copys the output data to the input data
        XferRequest* ptransfer;

        // Note that in the ActiveFlowControl mode we only send the state to indicate that our
        // buffer is ready for the remote actor to pull data.
        try {

          // Create the transfer that copys the output state to the remote input state
          ptransfer = 
            ptemplate->copy (
                             output_offsets->localStateOffset + s_port->getPortId() * sizeof(BufferState),
                             input_offsets->localStateOffset + s_port->getPortId() * sizeof(BufferState),
                             sizeof(BufferState),
                             XferRequest::LastTransfer, NULL);

        }
        catch( ... ) {
          FORMAT_TRANSFER_EC_RETHROW( s_port, t_port );
        }

        // Add the transfer 
        temp->addTransfer( ptransfer );

      } // end for each input buffer

    } // end for each output buffer

  }  // end for each input port

}





// This base class provides a default pattern for the input buffers which is to 
// braodcast a input buffers availability to all shadows
void TransferTemplateGeneratorPattern1AFC::createInputTransfers(PortSet* output, Port* input,
                                                      TransferController* cont )

{

  OCPI::OS::uint32_t n;

  // We need to create the transfers to tell all of our shadow ports that a buffer
  // became available, 
  OCPI::OS::uint32_t n_t_buffers = input->getBufferCount();

  // We need a transfer template to allow a transfer for each input buffer to its
  // associated shadows
  for ( OCPI::OS::uint32_t t_buffers=0; t_buffers<n_t_buffers; t_buffers++ ) {

    // input buffer
    InputBuffer* t_buf = input->getInputBuffer(t_buffers);
    int t_tid = t_buf->getTid();

    // Create a template
    OcpiTransferTemplate* temp = new OcpiTransferTemplateAFC(0);

#ifndef NDEBUG
    printf("*&*&* Adding template for tpid = %d, ttid = %d, template = %p\n", 
           input->getPortId(), t_tid, temp);
#endif

    //Add the template to the controller
    cont->addTemplate( temp, 0, 0, input->getPortId(), t_tid, false, TransferController::INPUT );

#ifdef NEEDED
    struct PortMetaData::InputPortBufferControlMap *input_offsets = 
      &input->getMetaData()->m_bufferData[t_tid].inputOffsets;
#endif

    // Since there may be multiple output ports on 1 processs, we need to make sure we dont send
    // more than 1 time
    int sent[MAX_PCONTRIBS];
    memset(sent,0,sizeof(int)*MAX_PCONTRIBS);

    // We need to setup a transfer for each shadow, they exist in the unique output circuits
    for ( n=0; n<output->getPortCount(); n++ ) {

      // Since the shadows only exist in the circuits with instances of real
      // output ports, the offsets are indexed via the output port ordinals.
      // If the output is co-located with us, no shadow exists.
      Port* s_port = output->getPort(n);
      int s_pid = s_port->getRealShemServices()->getEndPoint()->mailbox;

      if ( sent[s_pid] ) {
        continue;
      }

      // If we are creating a template for whole transfers, we do not recognize anything
      // but output port 0
      if ( (s_port->getPortSet()->getDataDistribution()->getMetaData()->distType == DataDistributionMetaData::parallel) &&
           s_port->getRank() != 0 ) {
        continue;
      }

      // Attach zero-copy for co-location
      if ( m_zcopyEnabled && s_port->supportsZeroCopy( input ) ) {
#ifndef NDEBUG
        printf("Adding Zery copy for input response\n");
#endif
        temp->addZeroCopyTransfer( NULL, t_buf );
      }

      sent[s_pid] = 1;


    } // end for each input buffer

  } // end for each output port

}



// In AFC mode, the shadow port is responsible for pulling the data from the real output port, and then
// Telling the output port that its buffer is empty.
void TransferTemplateGeneratorPattern1AFCShadow::createOutputTransfers( Port* s_port, PortSet* input,
                                                              TransferController* cont )
{

  // Since this is a whole output distribution, only port 0 of the output
  // set gets to do anything.
  if ( s_port->getPortSet()->getDataDistribution()->getMetaData()->distType == DataDistributionMetaData::parallel &&
       s_port->getRank() != 0 ) {
    return;
  }

  int n;
  PortSet* output = s_port->getPortSet();
  int n_s_buffers = output->getBufferCount();
  int n_t_buffers = input->getBufferCount();
  int n_t_ports = input->getPortCount();

  // We need a transfer template to allow a transfer from each output buffer to every
  // input buffer for this pattern.
  for ( int s_buffers=0; s_buffers<n_s_buffers; s_buffers++ ) {

    // output buffer
    OutputBuffer* s_buf = s_port->getOutputBuffer(s_buffers);
    int s_tid = s_buf->getTid();

    // We need a transfer template to allow a transfer to each input buffer
    for ( int t_buffers=0; t_buffers<n_t_buffers; t_buffers++ ) {

      // Get the input port
      Port* t_port = input->getPort(0);

      // input buffer
      InputBuffer* t_buf = t_port->getInputBuffer(t_buffers);
      int t_tid = t_buf->getTid();

      // Create a template
      OcpiTransferTemplate* temp = new OcpiTransferTemplateAFC(1);


      // Add the template to the controller, for this pattern the output port
      // and the input ports remains constant
#ifndef NDEBUG
      printf("output port id = %d, buffer id = %d, input id = %d\n", 
             s_port->getPortId(), s_tid, t_tid);
      printf("Template address = %p\n", temp);
#endif

      cont->addTemplate( temp, s_port->getPortId(),
                         s_tid, 0 ,t_tid, false, TransferController::OUTPUT );

      /*
       *  This transfer is used to mark the local input shadow buffer as full
       */

      // We need to setup a transfer for each input port. 
#ifndef NDEBUG
      printf("Number of input ports = %d\n", n_t_ports);
#endif

      for ( n=0; n<n_t_ports; n++ ) {

        // Get the input port
        Port* t_port = input->getPort(n);
        t_buf = t_port->getInputBuffer(t_buffers);

        struct PortMetaData::OutputPortBufferControlMap *output_offsets = 
          &s_port->getMetaData()->m_bufferData[s_tid].outputOffsets;

        struct PortMetaData::InputPortBufferControlMap *input_offsets = 
          &t_port->getMetaData()->m_bufferData[t_tid].inputOffsets;

        // We need to determine if this can be a Zero copy transfer.  If so, 
        // we dont need to create a transfer template
        if ( m_zcopyEnabled && s_port->supportsZeroCopy( t_port ) ) {

#ifndef NDEBUG
          printf("** ZERO COPY TransferTemplateGeneratorPattern1::createOutputTransfers from %p, to %p\n", s_buf, t_buf);
#endif
          temp->addZeroCopyTransfer( s_buf, t_buf );
          continue;
        }

        /* Attempt to get or make a transfer template */
        XferServices* ptemplate = XferFactoryManager::getFactoryManager().getService( 
                                                                 s_port->getEndpoint(), 
                                                                 t_port->getEndpoint() );
        if ( ! ptemplate ) {
          FORMAT_TRANSFER_EC_RETHROW( s_port, t_port );
        }

        // Create the transfer that copys the output data to the input data
        XferRequest* grouped_ptransfer;

        try {

          // Create the data buffer transfer
          XferRequest* ptransfer = 
            ptemplate->copy (
                             output_offsets->bufferOffset,
                             input_offsets->bufferOffset,
                             output_offsets->bufferSize,
                             XferRequest::FirstTransfer, NULL);

          // Create the transfer that copies the output meta-data to the input meta-data 
          XferRequest* ptransfer_2 =
            ptemplate->copy (
                             output_offsets->metaDataOffset + s_port->getPortId() * sizeof(BufferMetaData),
                             input_offsets->metaDataOffset + s_port->getPortId() * sizeof(BufferMetaData),
                             sizeof(OCPI::OS::int64_t),
                             XferRequest::DataOffset, ptransfer );

          grouped_ptransfer = Group( ptemplate, ptransfer, ptransfer_2 );

          // FIXME.  We need to allocate a separate local flag for this.  (most dma engines dont have
          // an immediate mode).
          // Create the transfer that copies our state to back to the output to indicate that its buffer
          // is now available for re-use.
          ptransfer = ptemplate->copy (
                                       input_offsets->localStateOffset + s_port->getPortId() * sizeof(BufferState),
                                       output_offsets->localStateOffset + s_port->getPortId() * sizeof(BufferState),
                                       sizeof(BufferState),
                                       XferRequest::LastTransfer, grouped_ptransfer);

          grouped_ptransfer = Group(ptemplate, grouped_ptransfer, ptransfer );

        }
        catch( ... ) {
          FORMAT_TRANSFER_EC_RETHROW( s_port, t_port );
        }

        // Add the transfer 
        temp->addTransfer( grouped_ptransfer );

      } // end for each input buffer

    } // end for each output buffer

  }  // end for each input port

}






/*
 *
 ************************************************************************************
 *
 * Pattern 2
 *
 ************************************************************************************
 *
 */

// Create transfers for input port
void 
TransferTemplateGeneratorPattern2::
createInputTransfers(PortSet* output, 
                      Port* input, 
                      TransferController* cont )

{

  // For this pattern, we can use the base class implementation which broadcasts a
  // input buffers availability
  TransferTemplateGenerator::createInputTransfers( output, input, cont );

}




// Create transfers for output port
void 
TransferTemplateGeneratorPattern2::
createOutputTransfers(Port* s_port, 
                      PortSet* input,
                      TransferController* cont  )
{

  /*
   *        For a WP / SW transfer, we need to be capable of transfering from any
   *  output buffer to any one input buffer.
   */

#ifndef NDEBUG
  printf("In createOutputTransfers, parrtern #2   \n");
#endif

  // Since this is a whole output distribution, only port 0 of the output
  // set gets to do anything.
  if ( s_port->getPortSet()->getDataDistribution()->getMetaData()->distType == DataDistributionMetaData::parallel &&
       s_port->getRank() != 0 ) {
    return;
  }

  int n;
  PortSet* output = s_port->getPortSet();
  int n_s_buffers = output->getBufferCount();
  int n_t_buffers = input->getBufferCount();
  int n_t_ports = input->getPortCount();

  // We need a transfer template to allow a transfer from each output buffer to every
  // input buffer for this pattern.
  for ( int s_buffers=0; s_buffers<n_s_buffers; s_buffers++ ) {

    // output buffer
    OutputBuffer* s_buf = s_port->getOutputBuffer(s_buffers);
    int s_tid = s_buf->getTid();

    // Now we need to create a template for each input port
    for ( n=0; n<n_t_ports; n++ ) {

      // Get the input port
      Port* t_port = input->getPort(n);

      // We need a transfer template to allow a transfer to each input buffer
      for ( int t_buffers=0; t_buffers<n_t_buffers; t_buffers++ ) {

        // Get the buffer for the current port
        // input buffer
        InputBuffer* t_buf = t_port->getInputBuffer(t_buffers);
        t_buf = t_port->getInputBuffer(t_buffers);
        int t_tid = t_buf->getTid();

        // Create a template
        OcpiTransferTemplate* temp = new OcpiTransferTemplate(2);

        // Add the template to the controller, for this pattern the output port
        // and the input ports remains constant

#ifndef NDEBUG
        printf("output port id = %d, buffer id = %d, input id = %d\n", 
               s_port->getPortId(), s_tid, t_tid);
        printf("Template address = %p\n", temp);
#endif
        cont->addTemplate( temp, s_port->getPortId(),
                           s_tid, t_port->getPortId() ,t_tid, false, TransferController::OUTPUT );

        struct PortMetaData::OutputPortBufferControlMap *output_offsets = 
          &s_port->getMetaData()->m_bufferData[s_tid].outputOffsets;

        struct PortMetaData::InputPortBufferControlMap *input_offsets = 
          &t_port->getMetaData()->m_bufferData[t_tid].inputOffsets;

        TDataInterface tdi(s_port,s_tid,t_port,t_tid);

        // We need to determine if this can be a Zero copy transfer.  If so, 
        // we dont need to create a transfer template
        bool standard_transfer = true;
        if ( m_zcopyEnabled && s_port->supportsZeroCopy( t_port ) ) {

#ifndef NDEBUG
          printf("** ZERO COPY TransferTemplateGeneratorPattern2::createOutputTransfers from %p, to %p\n", s_buf, t_buf);
#endif
          temp->addZeroCopyTransfer( s_buf, t_buf );
          standard_transfer = false;
        }

        /* Attempt to get or make a transfer template */
        XferRequest* ptransfer[2];
        XferRequest::Flags flags;
        XferServices* ptemplate = 0; // init to suppress warning...
        if ( standard_transfer ) {

          /* Attempt to get or make a transfer template */
          ptemplate = XferFactoryManager::getFactoryManager().getService( 
                                                     s_port->getEndpoint(), 
                                                     t_port->getEndpoint());
          if ( ! ptemplate ) {
            FORMAT_TRANSFER_EC_RETHROW( s_port, t_port );
          }
        }

        // Pre-data transfer hook
        ptransfer[0] = addTransferPreData( tdi );
        flags = ptransfer[0] ? XferRequest::None : XferRequest::FirstTransfer;


        // Create the transfer that copys the output data to the input data
        if ( standard_transfer ) {

          try {
            ptransfer[1] = ptemplate->copy (
                                            output_offsets->bufferOffset,
                                            input_offsets->bufferOffset,
                                            output_offsets->bufferSize,
                                            (XferRequest::Flags)(flags | XferRequest::DataOffset), ptransfer[0]);

            if ( ptransfer[0] ) { 
              ptransfer[0] = Group( ptemplate, ptransfer[0], ptransfer[1] );
            }
            else {
              ptransfer[0] = ptransfer[1];
            }
          }
          catch ( ... ) {
            FORMAT_TRANSFER_EC_RETHROW( s_port, t_port );
          }
  

        }

        // Post-data transfer hook
        ptransfer[1] = addTransferPostData(tdi);
        if ( ptransfer[1] && ptransfer[0]) { 
          ptransfer[0] = Group( ptemplate, ptransfer[0], ptransfer[1] );
        }
        else if ( ptransfer[1] ) {
          ptransfer[0] = ptransfer[1];
        }

        // Create the transfer that copys the output meta-data to the input meta-data
        if ( standard_transfer ) {

          try {
            ptransfer[1] = ptemplate->copy (
                                            output_offsets->metaDataOffset + s_port->getPortId() * sizeof(BufferMetaData),
                                            input_offsets->metaDataOffset + s_port->getPortId() * sizeof(BufferMetaData),
                                            sizeof(OCPI::OS::int64_t),
                                            XferRequest::None, ptransfer[0]);
            ptransfer[0] = Group( ptemplate, ptransfer[0], ptransfer[1] );
          }
          catch ( ... ) {
            FORMAT_TRANSFER_EC_RETHROW( s_port, t_port );
          }

        }

        // Pre-state transfer hook
        ptransfer[1] = addTransferPreState(  tdi );
        if ( ptransfer[1] && ptransfer[0] ) { 
          ptransfer[0] = Group( ptemplate, ptransfer[0], ptransfer[1] );
        }
        else if ( ptransfer[1] ) {
          ptransfer[0] = ptransfer[1];
        }

        // Create the transfer that copys the output state to the remote input state
        if ( standard_transfer ) {
          try {
            ptransfer[1] = ptemplate->copy (
                                            output_offsets->localStateOffset + s_port->getPortId() * sizeof(BufferState),
                                            input_offsets->localStateOffset + s_port->getPortId() * sizeof(BufferState),
                                            sizeof(BufferState),
                                            XferRequest::LastTransfer, ptransfer[0]);
            ptransfer[0] = Group(ptemplate, ptransfer[0], ptransfer[1] );
          }
          catch ( ... ) {
            FORMAT_TRANSFER_EC_RETHROW( s_port, t_port );
          }
        }

        // Add the transfer 
        if ( ptransfer[0] ) {
          temp->addTransfer( ptransfer[0] );
        }

      } // end for each input buffer

    } // end for each input port

  }  // end for each output buffer
}




/*
 *
 ************************************************************************************
 *
 * Pattern 3
 *
 ************************************************************************************
 *
 */
XferRequest* TransferTemplateGeneratorPattern3::addTransferPreState( TDataInterface& tdi)
{

#ifndef NDEBUG
  printf("*** In TransferTemplateGeneratorPattern3::addTransferPreState() \n");
#endif

  XferRequest* ptransfer[2];
  ptransfer[0] = ptransfer[1] = NULL;

  // We need to update all of the shadow buffers for all "real" output ports
  // to let them know that the input buffer for this input port has been allocated

  struct PortMetaData::InputPortBufferControlMap *input_offsets = 
    &tdi.t_port->getMetaData()->m_bufferData[tdi.t_tid].inputOffsets;

  int our_smb_id = tdi.s_port->getMailbox();

  PortSet *sps = static_cast<PortSet*>(tdi.s_port->getPortSet());
  for ( OCPI::OS::uint32_t n=0; n<sps->getPortCount(); n++ ) {

    Port* shadow_port = static_cast<Port*>(sps->getPortFromIndex(n));
    int idx = shadow_port->getMailbox();

    // We need to ignore self transfers
    if ( idx == our_smb_id) {
      continue;
    }

    // A shadow for a output may not exist if they are co-located
    if ( input_offsets->myShadowsRemoteStateOffsets[idx] != 0 ) {

#ifndef NDEBUG
      printf("TransferTemplateGeneratorPattern3::addTransferPreState mapping shadow offset to 0x%llx\n", 
             (long long)input_offsets->myShadowsRemoteStateOffsets[idx]);
#endif

      /* Attempt to get or make a transfer template */
      XferServices* ptemplate = XferFactoryManager::getFactoryManager().getService( 
                                                               tdi.s_port->getEndpoint(), 
                                                               shadow_port->getEndpoint() );
      if ( ! ptemplate ) {
        FORMAT_TRANSFER_EC_RETHROW( tdi.s_port, shadow_port );
      }

      try {

        // Create the transfer that copys the local shadow buffer state to the remote
        // shadow buffers state
        ptransfer[0] = ptemplate->copy (
                                        input_offsets->myShadowsRemoteStateOffsets[our_smb_id],
                                        input_offsets->myShadowsRemoteStateOffsets[idx],
                                        sizeof(BufferState),
                                        XferRequest::None, ptransfer[1]);

        if ( ptransfer[1] ) {
          ptransfer[1] = Group( ptemplate, ptransfer[0], ptransfer[1] );
        }
        else {
          ptransfer[1] = ptransfer[0];
        }

      }
      catch ( ... ) {
        FORMAT_TRANSFER_EC_RETHROW( tdi.s_port, shadow_port );
      }

    }
  }

  // Now we need to pass the output control baton onto the next output port
  int next_output = (tdi.s_port->getPortId() + 1) % sps->getPortCount();
  Port* next_sp = static_cast<Port*>(sps->getPortFromIndex(next_output));

  struct PortMetaData::OutputPortBufferControlMap *output_offsets = 
    &tdi.s_port->getMetaData()->m_bufferData[tdi.s_tid].outputOffsets;

  struct PortMetaData::OutputPortBufferControlMap *next_output_offsets = 
    &next_sp->getMetaData()->m_bufferData[tdi.s_tid].outputOffsets;

  /* Attempt to get or make a transfer template */
  XferServices* ptemplate = XferFactoryManager::getFactoryManager().getService( 
                                                           tdi.s_port->getEndpoint(), 
                                                           next_sp->getEndpoint() );
  if ( ! ptemplate ) {
    FORMAT_TRANSFER_EC_RETHROW( tdi.s_port, next_sp );
  }

  // Create the transfer from out output contol state to the next

  try {
    ptransfer[0] = ptemplate->copy (
                                    output_offsets->portSetControlOffset,
                                    next_output_offsets->portSetControlOffset,
                                    sizeof(OutputPortSetControl),
                                    XferRequest::LastTransfer, ptransfer[1]);

    if ( ptransfer[1] ) {
      ptransfer[1] = Group( ptemplate, ptransfer[0], ptransfer[1] );
    }
    else {
      ptransfer[1] = ptransfer[0];
    }
  }
  catch ( ... ) {
    FORMAT_TRANSFER_EC_RETHROW(  tdi.s_port, next_sp );
  }

  return ptransfer[1];
}



/*
 *
 ************************************************************************************
 *
 * Pattern 4
 *
 ************************************************************************************
 *
 */

// Create transfers for input port
void TransferTemplateGeneratorPattern4::createInputTransfers(PortSet* output, 
                                                              Port* input, 
                                                              TransferController* cont )

{
  // For this pattern, we can use the base class implementation which broadcasts a
  // input buffers availability
  TransferTemplateGenerator::createInputTransfers( output, input, cont );
}



// Create transfers for output port
void 
TransferTemplateGeneratorPattern4::
createOutputTransfers(Port* s_port, 
                      PortSet* input,
                      TransferController* cont  )
{

  /*
   *        For a WP / P(Parts) transfer, we need to be capable of transfering from any
   *  output buffer to all input port buffers.
   */

#ifndef NDEBUG
  printf("In TransferTemplateGeneratorPattern4::createOutputTransfers  \n");
#endif

  // Since this is a whole output distribution, only port 0 of the output
  // set gets to do anything.
  if ( s_port->getPortSet()->getDataDistribution()->getMetaData()->distType == DataDistributionMetaData::parallel &&
       s_port->getRank() != 0 ) {
    return;
  }

  int n;
  PortSet* output = s_port->getPortSet();
  int n_s_buffers = output->getBufferCount();
  int n_t_buffers = input->getBufferCount();
  int n_t_ports = input->getPortCount();

  DataPartition* dpart = input->getDataDistribution()->getDataPartition();

  // Get the number of transfers that it is going to take to satisfy this output
  int n_transfers_per_output_buffer = dpart->getTransferCount(output, input);

  // For this pattern we need a transfer count that is a least as large as the number
  // of input ports, since we have to inform everyone about the end of whole
  if ( m_markEndOfWhole ) {
    if ( n_transfers_per_output_buffer < n_t_ports ) {

      //                  n_transfers_per_output_buffer = n_t_ports;

    }
  }

  // Get the total number of parts that make up the whole
  int parts_per_whole = dpart->getPartsCount(output, input);

#ifndef NDEBUG
  printf("** There are %d transfers to complete this set\n", n_transfers_per_output_buffer);
  printf("There are %d parts per whole\n", parts_per_whole );
#endif

  // We need a transfer template to allow a transfer from each output buffer to every
  // input buffer for this pattern.
  int sequence;
  OcpiTransferTemplate *root_temp=NULL, *temp=NULL;
  for ( int s_buffers=0; s_buffers<n_s_buffers; s_buffers++ ) {

    // output buffer
    OutputBuffer* s_buf = s_port->getOutputBuffer(s_buffers);
    int s_tid = s_buf->getTid();
    int part_sequence;

    // We need a transfer template to allow a transfer to each input buffer
    for ( int t_buffers=0; t_buffers<n_t_buffers; t_buffers++ ) {

      //Each output buffer may need more than 1 transfer to satisfy itself
      sequence = 0;
      part_sequence = 0;
      for ( int transfer_count=0; transfer_count<n_transfers_per_output_buffer; 
            transfer_count++, sequence++ ) {

        // Get the input port
        Port* t_port = input->getPort(0);
        InputBuffer* t_buf = t_port->getInputBuffer(t_buffers);
        int t_tid = t_buf->getTid();

        // We need to be capable of transfering the gated transfers to all input buffers
        for ( int t_gated_buffer=0; t_gated_buffer<n_t_buffers+1; t_gated_buffer++ ) {

          // This may be gated transfer
          if ( (transfer_count == 0) && (t_gated_buffer == 0)) {

            temp = new OcpiTransferTemplate(4);
            root_temp = temp;

            // Add the template to the controller, 
            cont->addTemplate( temp, s_port->getPortId(),
                               s_tid, 0 ,t_tid, false, TransferController::OUTPUT );

          }
          else {
            part_sequence = transfer_count * n_t_ports;
            temp = new OcpiTransferTemplate(4);
            root_temp->addGatedTransfer( sequence, temp, 0, t_tid);
          }


          // We need to setup a transfer for each input port. 
          for ( n=0; n<n_t_ports; n++ ) {

            // Get the input port
            Port* t_port = input->getPort(n);
            t_buf = t_port->getInputBuffer(t_buffers);

            struct PortMetaData::OutputPortBufferControlMap *output_offsets = 
              &s_port->getMetaData()->m_bufferData[s_tid].outputOffsets;

            struct PortMetaData::InputPortBufferControlMap *input_offsets = 
              &t_port->getMetaData()->m_bufferData[t_tid].inputOffsets;

            // Since this is a "parts" transfer, we dont allow zero copy

            /* Attempt to get or make a transfer template */
            XferServices* ptemplate = XferFactoryManager::getFactoryManager().getService( 
                                                                     s_port->getEndpoint(), 
                                                                     t_port->getEndpoint() );
            if ( ! ptemplate ) {
              FORMAT_TRANSFER_EC_RETHROW( s_port, t_port );
            }

            // Now we need to go to the data partition class and ask for some offsets
            DataPartition::BufferInfo *bi_tmp, *buffer_info;
            dpart->calculateBufferOffsets( transfer_count, s_buf, t_buf, &buffer_info);

            XferRequest* ptransfer[2];
            bi_tmp = buffer_info;
            ptransfer[0] = ptransfer[1] = NULL;
            int total_bytes=0;
            while ( bi_tmp ) {

              ptransfer[1] = ptransfer[0];

              try {

                // Create the transfer that copys the output data to the input data
                ptransfer[0] = ptemplate->copy (
                                                output_offsets->bufferOffset + bi_tmp->output_offset,
                                                input_offsets->bufferOffset + bi_tmp->input_offset,
                                                bi_tmp->length,
                                                XferRequest::FirstTransfer, NULL);

                total_bytes += bi_tmp->length;

                // Group the transfers if needed
                if ( ptransfer[0] && ptransfer[1] ) {
                  ptransfer[0] =  Group( ptemplate, ptransfer[0], ptransfer[1] );
                  ptransfer[1] = NULL;
                }

              }
              catch ( ... ) {
                FORMAT_TRANSFER_EC_RETHROW( s_port, t_port );
              }

              bi_tmp = bi_tmp->next;
            }
            delete buffer_info;

            // At this point we need to tell the template what needs to be inserted into
            // the output meta-data prior to transfer, this includes the actual number
            // of bytes that were transfered and the end of whole indicator.
            bool end_of_whole;
            if (  transfer_count == (n_transfers_per_output_buffer-1) ) {
              end_of_whole = true;
            }
            else {
              end_of_whole = false;
            }
            temp->presetMetaData( s_buf->getMetaDataByIndex( t_port->getPortId() ),
                                  total_bytes, end_of_whole, parts_per_whole, part_sequence++ );


            try {

              // Create the transfer that copys the output meta-data to the input meta-data
              ptransfer[1] = ptemplate->copy (
                                              output_offsets->metaDataOffset + t_port->getPortId() * sizeof(BufferMetaData),
                                              input_offsets->metaDataOffset + s_port->getPortId() * sizeof(BufferMetaData),
                                              sizeof(OCPI::OS::uint64_t),
                                              XferRequest::None, ptransfer[0]);
              ptransfer[0] = Group( ptemplate, ptransfer[0], ptransfer[1] );

              // Create the transfer that copys the output state to the remote input state
              ptransfer[1] = ptemplate->copy (
                                              output_offsets->localStateOffset + s_port->getPortId() * sizeof(BufferState),
                                              input_offsets->localStateOffset + s_port->getPortId() * sizeof(BufferState),
                                              sizeof(BufferState),
                                              XferRequest::LastTransfer, ptransfer[0]);

              ptransfer[0] = Group( ptemplate, ptransfer[0], ptransfer[1] );
            }
            catch ( ... ) {
              FORMAT_TRANSFER_EC_RETHROW( s_port, t_port );
            }

            // Add the transfer 
            temp->addTransfer( ptransfer[0] );

          } // end for each input port

          t_buf = t_port->getInputBuffer(t_gated_buffer%n_t_buffers);
          t_tid = t_buf->getTid();

        } // for each gated buffer

      } // end for each input buffer

    } // end for n transfers

  }  // end for each output buffer 

}


