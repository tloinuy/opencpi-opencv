
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
 *   Container application context class.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 3/2005
 *    Revision Detail: Created
 *
 */

#define WORKER_INTERNAL
#include <OcpiCP289Port.h>
#include <OcpiCP289Container.h>
#include <OcpiCP289Controller.h>
#include <OcpiCP289Application.h>
#include <OcpiOsMisc.h>
#include <OcpiTransport.h>
#include <OcpiRDTInterface.h>
#include <OcpiOsAssert.h>
#include <OcpiUtilCDR.h>
#include <OcpiPortMetaData.h>
#include <OcpiUtilAutoMutex.h>
#include <OcpiContainerErrorCodes.h>
#include <DtTransferInternal.h>
#include <OcpiIntParallelDataDistribution.h>
#include <OcpiPValue.h>
#include <OcpiUtilMisc.h>
#include <OcpiParentChild.h>
#include <OcpiMetadataWorker.h>
#include <OcpiProperty.h>

#define SET_LAST_ERROR_TO_WORKER_ERROR_STRING(x)                        \
  if ( x->m_rcc_worker->m_context->errorString ) {                                \
    x->m_lastError=x->m_rcc_worker->m_context->errorString;                        \
  }                                                                        \
  else {                                                                \
    x->m_lastError = "Worked returned failure, but did not set errorString"; \
  }                                                                        \
  x->m_rcc_worker->m_context->errorString=NULL;


#define SET_LAST_ERROR_UNKNOWN_WORKER(x) x->m_lastError = "Unknown Worker Type"
#define SET_LAST_ERROR_WORKER_IN_UNUSABLE_STATE(x) x->m_lastError = "Worker in Unusable State"
#define SET_LAST_ERROR_INVALID_SEQUENCE(x) x->m_lastError = "Invalid Control Sequence"
#define SET_LAST_ERROR_ALL_REQUIRED_PORTS_NOT_CONNECTED(x) x->m_lastError = "Not all required ports are connected"
#define SET_LAST_ERROR_PORT_COUNT_MISMATCH(x) x->m_lastError = "Input/Output port count mismatch between worker and meta-data"
#define SET_LAST_ERROR_TO_TEST_NOT_IMPLEMENTED(x) x->m_lastError = "Test Not Implemented"
#define SET_LAST_ERROR_TO_PROPTERY_OVERRUN(x) x->m_lastError = "Property Overrun error"

using namespace OCPI::Container;
using namespace OCPI::Util;
using namespace OCPI::CP289;
namespace CM = OCPI::Metadata;
namespace CP = OCPI::Util::Prop;

#define MyParent static_cast<OCPI::CP289::Application*>(myParent)

void 
OCPI::CP289::Worker::
overRidePortInfo( OCPI::Metadata::Port & portData )
{
  RCCPortInfo* pi = NULL;
  if ( m_rcc_worker->m_dispatch->portInfo == NULL ) {
    return;
  }
  int n=0;
  while ( m_rcc_worker->m_dispatch->portInfo[n].port != RCC_NO_ORDINAL ) {
    if ( m_rcc_worker->m_dispatch->portInfo[n].port == portData.m_pid  ) {
      pi = &m_rcc_worker->m_dispatch->portInfo[n];
      break;
    }
    n++;
  }
  if ( pi ) {
#ifndef NDEBUG
    printf("\nWorker Port info is non NULL, overriding port defaults from s,bc -> %d,%d to %d,%d\n",
           portData.minBufferCount, portData.minBufferSize, pi->minBuffers, pi->maxLength );
#endif
    portData.minBufferCount = ( portData.minBufferCount > pi->minBuffers ) ? portData.minBufferCount : pi->minBuffers;
    portData.minBufferSize  = portData.maxBufferSize  = pi->maxLength;

  }
}



OCPI::CP289::Worker::
Worker( OCPI::Container::Application & app, RCCDispatch* wd, OCPI::Util::PValue *wparams,
        ::RCCContainer * rcc_container, ezxml_t impl, ezxml_t inst)
  : OCPI::Container::Worker(app,impl,inst),
   m_rcc_worker(0),workerId(NULL),enabled(false),sourcePortCount(0),targetPortCount(0),
  sourcePorts(1), targetPorts(1), runConditionSS(0), worker_run_count(0)
{
  ( void ) wparams;
  workerId = reinterpret_cast<WorkerId>(this);
#if 0
  // This won't work.
  if ( wparams ) {
    properties = *wparams;
  }
#endif
  m_rcc_worker = new OCPI::CP289::RCCWorkerInterface( wd, this );
  m_rcc_worker->m_context->container = rcc_container;

  // If we have an event handler, we need to inform it about the timeout
  if (  wd->runCondition && wd->runCondition->timeout ) {
    if ( MyParent->getTransport().m_transportGlobal->getEventManager() ) {

#ifdef EM_PORT_COMPLETE
      MyParent->myparent->m_transport->m_transportGlobal->getEventManager()->setMinTimeout( workerId, 
                                                                        wd->runCondition->usecs );
#endif

    }
  }

  m_wciStatus.event = WCI_EVENT_NOP;
  m_wciStatus.pending = false;
  m_wciStatus.error = WCI_SUCCESS;
}

void 
OCPI::CP289::Worker::
stop( bool informWorker )
{
  enabled = false;
  if ( informWorker ) {
    RCCResult result = m_rcc_worker->stop();
    if ( result != RCC_OK ) {
      throw OCPI::Util::EmbeddedException( "Worker disable failed" );
    }
  }
}


void 
OCPI::CP289::Worker::
getProperties( 
              OCPI::OS::uint32_t offset, 
              OCPI::OS::uint32_t nBytes, 
              void * p_data  )
{
  if ( (offset+nBytes) > m_rcc_worker->m_dispatch->propertySize ) {
    throw OCPI::Util::EmbeddedException( PROPERTY_GET_EXCEPTION, NULL, ApplicationRecoverable);
  }
  memcpy( p_data, (char*)m_rcc_worker->m_context->properties+offset, nBytes );
}

void 
OCPI::CP289::Worker::
setProperties( 
              OCPI::OS::uint32_t offset, 
              OCPI::OS::uint32_t nBytes, 
              const void * p_data  )
{
  if ( (offset+nBytes) > m_rcc_worker->m_dispatch->propertySize ) {
    throw OCPI::Util::EmbeddedException( PROPERTY_SET_EXCEPTION, NULL, ApplicationRecoverable);
  }
  memcpy( (char*)m_rcc_worker->m_context->properties+offset, p_data, nBytes );
}







// Destructor
OCPI::CP289::RCCWorkerInterface::
~RCCWorkerInterface()
{
  for ( int n=0; n<m_nports; n++ ) {
    OCPI::CP289::OpaquePortData *opd = static_cast<OCPI::CP289::OpaquePortData*>(m_context->ports[n].opaque);
    delete opd;
  }

  OCPI::OS::uint32_t m = 0;
  while ( m_context->memories && m_context->memories[m] ) {
    delete [] (char*)m_context->memories[m];
    m++;
  }
  delete[] m_context->memories;
  delete[] m_context->runCondition->portMasks;
  delete m_context->runCondition;
  delete[] (char*)m_context->properties;
  delete[] m_context;

}


OCPI::DataTransport::Transport &
OCPI::CP289::Worker::
getTransport()
{
  return MyParent->getTransport();
}

OCPI::CP289::Worker::
~Worker()
{

  try {
    stop(true);
  }
  catch( ... ) {
    // Ignore
  }

  if ( !m_rcc_worker->releaseImplemented() ) {
    ( void )m_rcc_worker->release();
  }

#ifdef EM_PORT_COMPLETE
    RCCDispatch* wd = m_rcc_worker->m_dispatch;
    // If we have an event handler, we need to inform it about the timeout
    if ( wd->runCondition && wd->runCondition->timeout ) {
      if ( MyParent->m_transport->m_transportGlobal->getEventManager() ) {
        MyParent->m_transport->m_transportGlobal->getEventManager()->removeMinTimeout( w->workerId );
      }
    }
#endif

  delete m_rcc_worker;

}



// Constructor
OCPI::CP289::RCCWorkerInterface::
RCCWorkerInterface( RCCDispatch * wd, OCPI::CP289::Worker * wi )
  :m_state(OCPI::CP289::RCCWorkerInterface::WorkerExists), m_dispatch(wd)
{        
 
  // Create our memory spaces
  int idx=0;
  while( wd->memSizes && wd->memSizes[idx] ) {
#ifndef NDEBUG
    printf("Allocating %d bytes of data for worker memory\n", wd->memSizes[idx] );
#endif
    idx++;
  }
  int mcount = idx;

  // Create our context
  m_nports = wd->numInputs+wd->numOutputs;
  m_context = (RCCWorker *)new char[sizeof(RCCWorker)+(m_nports)*sizeof(RCCPort)];
  m_context->properties = 0;
  if ( mcount ) {
    try {
      m_context->memories = new void*[mcount+1];
    }
    catch( std::bad_alloc ) {
      delete[] m_context;
      throw OCPI::Util::EmbeddedException( NO_MORE_MEMORY, "worker requested too much memory", ApplicationRecoverable);
    }
    m_context->memories[mcount] = NULL;
  }
  else {
    m_context->memories = NULL;
  }
  
  m_context->connectedPorts = 0;
  m_context->runCondition = new RCCRunCondition;
 
  int rc_count=0;
  if ( wd->runCondition ) {

    if ( wd->runCondition->portMasks ) {
      while( wd->runCondition->portMasks[rc_count] ) rc_count++;

      // Add one for the zero based terminator
      rc_count++;

      m_context->runCondition->portMasks = new RCCPortMask[rc_count];
      memcpy( m_context->runCondition->portMasks, wd->runCondition->portMasks, sizeof(RCCPortMask)*rc_count);
    }  
    else { // portMask is NULL, per CP289 spec, timeout only run condition
      m_context->runCondition->portMasks = new RCCPortMask[1];
      m_context->runCondition->portMasks[0] = 0;
    }
    
    if ( wd->runCondition->timeout ) {
      wi->runTimeout.seconds = wd->runCondition->usecs / 1000000;
      wi->runTimeout.nanoseconds = (wd->runCondition->usecs%1000000) * 1000;
    }
  }
  else {
    rc_count = 1;
    m_context->runCondition->portMasks = new RCCPortMask[2];
    m_context->runCondition->portMasks[0] = 0;
    m_context->runCondition->portMasks[1] = 0;

    for ( int n=0; n<(wd->numInputs+wd->numOutputs); n++ ) {
      m_context->runCondition->portMasks[0] |=  (1<<n);
    }
  }


  // This is the superset of the run conditions
  wi->runConditionSS = 0;
  for (int n=0; n< rc_count; n++ ) {
    wi->runConditionSS |= m_context->runCondition->portMasks[n];
  }


  // We create one of these structures for all of the ports that are defined in the worker.  However,
  // the actual data ports may be optional at runtime.
  for ( int n=0; n<m_nports; n++ ) {
    m_context->ports[n].opaque = new OCPI::CP289::OpaquePortData();
    m_context->ports[n].current.data = 0;
    m_context->ports[n].current.maxLength = 0;
    m_context->ports[n].callBack = 0;
    static_cast<OCPI::CP289::OpaquePortData*>(m_context->ports[n].opaque)->worker = wi;
    static_cast<OCPI::CP289::OpaquePortData*>(m_context->ports[n].opaque)->cp289Port = &m_context->ports[n];
  }

  idx=0;
  while( wd->memSizes && wd->memSizes[idx] ) {
    try {
      m_context->memories[idx] = new char*[wd->memSizes[idx]];
    }
    catch( std::bad_alloc ) {
      delete[] m_context;
      throw OCPI::Util::EmbeddedException( NO_MORE_MEMORY, "worker requested too much memory", ApplicationRecoverable);
    }
    idx++;
  }
  
  // Now create and initialize the worker properties
  m_context->properties = new char[wd->propertySize + 4];

  memset(m_context->properties, 0, sizeof(char)*wd->propertySize);
        
}

OCPI::Container::Port & 
OCPI::CP289::Worker::
createPort(OCPI::Metadata::Port& mp )
{

  int bSize = ( mp.minBufferSize == 0 ) ? OCPI::Metadata::Port::DEFAULT_BUFFER_SIZE : mp.minBufferSize;
  int bCount = ( mp.minBufferCount == 0 ) ? OCPI::Metadata::Port::DEFAULT_NBUFFERS : mp.minBufferCount;
  if ( mp.provider ) {
    return createInputPort( mp.m_pid, bCount, bSize, NULL );
  }
  else {
    return createOutputPort( mp.m_pid, bCount, bSize, NULL );
  }

}




OCPI::Container::Port &
OCPI::CP289::Worker::
createOutputPort( 
                 PortId               portId,
                 OCPI::OS::uint32_t    bufferCount,
                 OCPI::OS::uint32_t    bufferSize, 
                 OCPI::Util::PValue*              props            
                 )
  throw ( OCPI::Util::EmbeddedException )
{
  ( void ) props;
  TRACE(" OCPI::CP289::Container::createOutputPort()");
  OCPI::Util::AutoMutex guard ( MyParent->mutex(), true ); 

  sourcePortCount++;

  if ( sourcePortCount > m_rcc_worker->m_dispatch->numOutputs  ) {
    throw OCPI::Util::EmbeddedException( PORT_NOT_FOUND, "Source Port count exceeds configuration", ApplicationFatal);
  }
  if ( portId >= (m_rcc_worker->m_dispatch->numInputs + m_rcc_worker->m_dispatch->numOutputs) ) {
    throw OCPI::Util::EmbeddedException( PORT_NOT_FOUND, "Port id exceeds port count", ApplicationFatal);
  }
  if ( portId > 32 ) {
    throw OCPI::Util::EmbeddedException( PORT_NOT_FOUND, "Port id exceeds max port count of 32", ApplicationFatal);
  }

  OCPI::Metadata::Port pmd;
  pmd.m_pid = portId;
  pmd.provider = false;
  pmd.minBufferCount = bufferCount;
  pmd.minBufferSize = bufferSize;

  // Check to see if the worker requested specific port information
  overRidePortInfo( pmd );

  Port *port;
  try {
    port = new OCPI::CP289::Port(*this, pmd, NULL );
  }
  catch( std::bad_alloc ) {
    throw OCPI::Util::EmbeddedException( NO_MORE_MEMORY, "new", ContainerFatal);
  }

  m_rcc_worker->m_context->ports[portId].current.maxLength = 
    (pmd.minBufferSize > pmd.maxBufferSize) ? pmd.minBufferSize : pmd.maxBufferSize;

  // Defer the real work until the port is connected.
  sourcePorts.insertToPosition(port, portId);

  return *port;
}




OCPI::Container::Port &
OCPI::CP289::Worker::
createInputPort( 
                 PortId              portId,   
                 OCPI::OS::uint32_t   bufferCount,
                 OCPI::OS::uint32_t   bufferSize, 
                 OCPI::Util::PValue*             props            
                 )
  throw ( OCPI::Util::EmbeddedException )
{
  TRACE("OCPI::CP289::Container::createInputPort()");
  OCPI::Util::AutoMutex guard ( MyParent->mutex(), true ); 
  if ( (bufferSize == 0) || (bufferCount == 0) ) {
    throw OCPI::Util::EmbeddedException( BAD_PORT_CONFIGURATION, "0 buffer count or size", ApplicationRecoverable);
  }
  targetPortCount++;
  if ( targetPortCount > m_rcc_worker->m_dispatch->numInputs  ) {
    throw OCPI::Util::EmbeddedException( PORT_NOT_FOUND, "Target Port count exceeds configuration", ApplicationFatal);
  }
  if ( portId >= (m_rcc_worker->m_dispatch->numInputs + m_rcc_worker->m_dispatch->numOutputs) ) {
    throw OCPI::Util::EmbeddedException( PORT_NOT_FOUND, "Port id exceeds port count", ApplicationFatal);
  }
  if ( portId > 32 ) {
    throw OCPI::Util::EmbeddedException( PORT_NOT_FOUND, "Port id exceeds max port count of 32", ApplicationFatal);
  }

  // The caller can also be less specific and just specify the protocol 
  std::string protocol;
  std::string endpoint;
  const OCPI::Util::PValue* p = OCPI::Util::PValue::find(props, "protocol");
  if ( p && (p->type != OCPI::Util::Prop::Scalar::OCPI_String)) {
    throw OCPI::Util::EmbeddedException("\"protocol\" property has wrong type, should be String");
  }
  if (p) {
    protocol = p->vString;
    endpoint = MyParent->getTransport().getEndpointFromProtocol( protocol.c_str() );
  }
  else if ( getenv("OCPI_DEFAULT_PROTOCOL") ) {
    
    const char * pe = getenv("OCPI_DEFAULT_PROTOCOL");
    printf("&&&& $$$$ Forcing protocol = %s \n", pe);
    protocol = pe;
    endpoint = MyParent->getTransport().getEndpointFromProtocol( protocol.c_str() );

  }
  else {

    // It is up to the caller to specify the endpoint which implicitly defines
    // the QOS.  If the caller does not provide the endpoint, we will pick one
    // by default.
    p = OCPI::Util::PValue::find(props, "endpoint");
    if ( p && (p->type != OCPI::Util::Prop::Scalar::OCPI_String)) {
      throw OCPI::Util::EmbeddedException("\"endpoint\" property has wrong type, should be String");
    }
    if (! p) {
      endpoint = MyParent->getTransport().getDefaultEndPoint();
    }
    else{
      endpoint = p->vString;
    }
    endpoint = MyParent->getTransport().addLocalEndpoint( endpoint.c_str() )->sMemServices->getEndPoint()->end_point;
  }


  OCPI::Metadata::Port pmd;
  pmd.m_pid = portId;
  pmd.provider = true;
  pmd.minBufferCount = bufferCount;
  pmd.minBufferSize = bufferSize;

  // Check to see if the worker requested specific port information
  overRidePortInfo( pmd );

  Port *port;
  try {
    port = new OCPI::CP289::Port(*this, pmd, endpoint.c_str() );
  }
  catch( std::bad_alloc ) {
    throw OCPI::Util::EmbeddedException( NO_MORE_MEMORY, "new", ContainerFatal);
  }

  // Add the port to the worker
  OpaquePortData * opaque =   
    static_cast<OCPI::CP289::OpaquePortData*>(m_rcc_worker->m_context->ports[portId].opaque);  
  port->opaque() = opaque;
  opaque->port = port;
  m_rcc_worker->m_context->ports[portId].current.maxLength = 
    (pmd.minBufferSize > pmd.maxBufferSize) ? pmd.minBufferSize : pmd.maxBufferSize;
  targetPorts.insertToPosition(port, portId);

  return *port;
}


void 
OCPI::CP289::Worker::
updatePort( OCPI::CP289::Port &port )
{
  // Add the port to the worker
  OCPI::CP289::OpaquePortData* opaque =
    static_cast<OCPI::CP289::OpaquePortData*>(m_rcc_worker->m_context->ports[port.portId()].opaque);
  port.opaque() = opaque;
  opaque->port = &port;
  opaque->buffer = NULL;
  opaque->readyToAdvance=1;
  m_rcc_worker->m_context->ports[port.portId()].current.data = NULL;
  m_rcc_worker->m_context->ports[port.portId()].current.maxLength = 
    opaque->port->dtPort()->getPortSet()->getBufferLength();
  m_rcc_worker->m_context->connectedPorts |= (1<<port.portId());

}


std::string 
OCPI::CP289::Worker::
getLastControlError()
  throw ( OCPI::Util::EmbeddedException )
{
  return m_lastError;
}



WCI_error 
OCPI::CP289::Worker::
control ( WCI_control operation, WCI_options options )
{
  ( void ) options;
  OCPI::Util::AutoMutex guard ( MyParent->mutex(), true ); 

  m_wciStatus.event = WCI_EVENT_CONTROL;
  m_wciStatus.info.control = operation;
  m_wciStatus.error = WCI_SUCCESS;
  RCCResult rc;

  if ( m_rcc_worker->getState() == RCCWorkerInterface::WorkerUnusable ) {
#ifndef NDEBUG
    printf("Component::control() Unusable Worker for operation %d\n", operation);
#endif
    m_wciStatus.error =   WCI_ERROR_UNUSABLE_WORKER;
    SET_LAST_ERROR_WORKER_IN_UNUSABLE_STATE(this);
    return m_wciStatus.error;      
  }


  switch( operation ) {

  case WCI_CONTROL_INITIALIZE: 
    {
      if ( m_rcc_worker->getState() != RCCWorkerInterface::WorkerExists ) {
        m_wciStatus.error = WCI_ERROR_INVALID_CONTROL_SEQUENCE;
        SET_LAST_ERROR_INVALID_SEQUENCE(this);
        break;
      }
      rc = m_rcc_worker->initialize();
      if ( rc == RCC_OK ) {
        // Nothing to do here
      }
      else if ( rc == RCC_ERROR ) {
        m_wciStatus.error = WCI_ERROR_UNKNOWN;
        SET_LAST_ERROR_TO_WORKER_ERROR_STRING(this);
      }
      else {
        m_wciStatus.error = WCI_ERROR_UNUSABLE_WORKER;
        SET_LAST_ERROR_TO_WORKER_ERROR_STRING(this);
      }
    }
    break;

  case WCI_CONTROL_START:
    {
      if ( (m_rcc_worker->getState() != RCCWorkerInterface::WorkerInitialized) &&
           (m_rcc_worker->getState() != RCCWorkerInterface::WorkerSuspended) ) {
        m_wciStatus.error = WCI_ERROR_INVALID_CONTROL_SEQUENCE;
        SET_LAST_ERROR_INVALID_SEQUENCE(this);
        break;
      }
        
      // If a worker gets started before all of its ports are created, return 
      // an error
      if ( (int)(targetPortCount + sourcePortCount) != 
           (int)(m_rcc_worker->m_dispatch->numInputs + m_rcc_worker->m_dispatch->numOutputs ) ) {
        m_wciStatus.error = WCI_ERROR_INVALID_CONTROL_SEQUENCE;
        SET_LAST_ERROR_PORT_COUNT_MISMATCH(this);
        break;
      }

      // If the worker does not have all the required ports connected, return an
      // error
      OCPI::CP289::Port* port = static_cast<OCPI::CP289::Port*>(firstChild());
      while ( port ) {
        RCCPortMask pm = 1<<port->portId();
        if ( m_rcc_worker->m_dispatch->optionallyConnectedPorts & pm ) {
          port = static_cast<OCPI::CP289::Port*>(nextChild(port));
          continue;
        }
        OCPI::DataTransport::Circuit* c = port->circuit();
        if ( c ) {
          if ( c->isCircuitOpen() ) {
            m_wciStatus.error = WCI_ERROR_INVALID_CONTROL_SEQUENCE;
            SET_LAST_ERROR_ALL_REQUIRED_PORTS_NOT_CONNECTED(this); 
            break;
          }
        }
        else {
          m_wciStatus.error = WCI_ERROR_INVALID_CONTROL_SEQUENCE;
          SET_LAST_ERROR_ALL_REQUIRED_PORTS_NOT_CONNECTED(this); 
          break;
        }        
        port = static_cast<OCPI::CP289::Port*>(nextChild(port));
      }



      if (   m_wciStatus.error == WCI_SUCCESS ) {
        rc = m_rcc_worker->start();
        if ( rc == RCC_OK ) {
          enabled = true;
          runTimer.start();
        }
        else if ( rc == RCC_ERROR ) {
          m_wciStatus.error = WCI_ERROR_UNKNOWN;
          SET_LAST_ERROR_TO_WORKER_ERROR_STRING(this);
        }
        else {
          m_wciStatus.error = WCI_ERROR_UNUSABLE_WORKER;
          SET_LAST_ERROR_TO_WORKER_ERROR_STRING(this);
        }
      }
    }
    break;

  case WCI_CONTROL_STOP:
    {
      if ( m_rcc_worker->getState() != RCCWorkerInterface::WorkerOperational ) {
        m_wciStatus.error = WCI_ERROR_INVALID_CONTROL_SEQUENCE;
        SET_LAST_ERROR_INVALID_SEQUENCE(this);
        break;
      }

      // The worker gets disabled here regardless if it complains
      enabled = false;
      rc = m_rcc_worker->stop();
      if ( rc == RCC_OK ) {
        runTimer.stop();
        runTimer.reset();
      }
      else if ( rc == RCC_ERROR ) {
        m_wciStatus.error = WCI_ERROR_UNKNOWN;
        SET_LAST_ERROR_TO_WORKER_ERROR_STRING(this);
      }
      else {
        m_wciStatus.error = WCI_ERROR_UNUSABLE_WORKER;
        SET_LAST_ERROR_TO_WORKER_ERROR_STRING(this);
      }

    }
    break;

  case WCI_CONTROL_RELEASE:
    if ( (m_rcc_worker->getState() != RCCWorkerInterface::WorkerExists) &&
         (m_rcc_worker->getState() != RCCWorkerInterface::WorkerInitialized) &&
         (m_rcc_worker->getState() != RCCWorkerInterface::WorkerOperational) &&
         (m_rcc_worker->getState() != RCCWorkerInterface::WorkerSuspended) ) {
      m_wciStatus.error = WCI_ERROR_INVALID_CONTROL_SEQUENCE;
      SET_LAST_ERROR_INVALID_SEQUENCE(this);
      break;
    }
    if (  ! m_rcc_worker->releaseImplemented() ) {
      break;
    }
    rc = m_rcc_worker->release();
    if ( rc == RCC_OK ) {
      // Nothing to do here
    }
    else if ( rc == RCC_ERROR ) {
      m_wciStatus.error = WCI_ERROR_UNKNOWN;
      SET_LAST_ERROR_TO_WORKER_ERROR_STRING(this);
    }
    else {
      m_wciStatus.error = WCI_ERROR_UNUSABLE_WORKER;
      SET_LAST_ERROR_TO_WORKER_ERROR_STRING(this);
    }
    break;

  case WCI_CONTROL_TEST:
    if (  ! m_rcc_worker->testImplemented() ) {
      m_wciStatus.error = WCI_ERROR_TEST_NOT_IMPLEMENTED;
      SET_LAST_ERROR_TO_TEST_NOT_IMPLEMENTED(this);
      break;
    }
    rc = m_rcc_worker->test();
    if ( rc != RCC_OK ) {
      SET_LAST_ERROR_TO_WORKER_ERROR_STRING(this);
      m_wciStatus.error = WCI_ERROR_UNKNOWN;
    }
    break;

  case WCI_CONTROL_AFTER_CONFIG:
    rc = m_rcc_worker->afterConfigure();
    if ( rc != RCC_OK ) {
      SET_LAST_ERROR_TO_WORKER_ERROR_STRING(this);
      m_wciStatus.error = WCI_ERROR_UNKNOWN;
    }
    break;

  case WCI_CONTROL_BEFORE_QUERY:
    rc = m_rcc_worker->beforeQuery();
    if ( rc != RCC_OK ) {
      SET_LAST_ERROR_TO_WORKER_ERROR_STRING(this);
      m_wciStatus.error = WCI_ERROR_UNKNOWN;
    }
    break;

  };


#ifndef NDEBUG
  if ( m_wciStatus.error != WCI_SUCCESS ) {
    printf("Component::control() Returning error code(%d) for operation %d\n", m_wciStatus.error, operation);
    printf("error string = %s\n", m_lastError.c_str() );
  }
#endif


#ifdef EM_PORT_COMPLETE
  // If we are event driven, we need to wake up the event manager to see if there is
  // work to be done.
  if ( m_transport->m_transportGlobal->useEvents() ) {
    DataTransfer::EndPoint* ep = m_transport->getEndpoint();
    m_transport->m_transportGlobal->getEventManager()->wakeUp(ep);
  }
#endif

  return m_wciStatus.error;         

}


WCI_error 
OCPI::CP289::Worker::
status ( WCI_status* p_status )
{
  OCPI::Util::AutoMutex guard ( MyParent->mutex(), true ); 
  *p_status = m_wciStatus;
  return WCI_SUCCESS;
}


WCI_error 
OCPI::CP289::Worker::
read ( WCI_u32 offset,
       WCI_u32 nBytes,
       WCI_data_type data_type,
       WCI_options options,
       void* p_data )
{
  ( void ) data_type;
  ( void ) options;
  OCPI::Util::AutoMutex guard ( MyParent->mutex(), true ); 

  m_wciStatus.event = WCI_EVENT_READ;
  m_wciStatus.info.offset = offset;
  m_wciStatus.info.overrun  = 0;
  m_wciStatus.error = WCI_SUCCESS;
  try {
    getProperties( offset, nBytes, p_data );
  }
  catch ( ... ) {
    m_wciStatus.info.overrun = offset;
    SET_LAST_ERROR_TO_PROPTERY_OVERRUN(this);
    m_wciStatus.error = WCI_ERROR_PROPERTY_OVERRUN;
    return m_wciStatus.error;
  }
  return m_wciStatus.error;

}


WCI_error 
OCPI::CP289::Worker::
write ( WCI_u32 offset,
        WCI_u32 nBytes,
        WCI_data_type data_type,
        WCI_options options,
        const void* p_data )
{
  ( void ) data_type;
  ( void ) options;
  OCPI::Util::AutoMutex guard ( MyParent->mutex(), true ); 

  m_wciStatus.event = WCI_EVENT_WRITE;
  m_wciStatus.info.offset = offset;
  m_wciStatus.info.overrun  = 0;
  m_wciStatus.error = WCI_SUCCESS;
  try {
    setProperties( offset, nBytes, p_data );
  }
  catch ( ... ) {
    m_wciStatus.info.overrun = offset;
    SET_LAST_ERROR_TO_PROPTERY_OVERRUN(this);
    m_wciStatus.error = WCI_ERROR_PROPERTY_OVERRUN;
    return m_wciStatus.error;
  }
  return m_wciStatus.error;

}



WCI_error 
OCPI::CP289::Worker::
close (        WCI_options options ) 
{
  ( void ) options;
  // No - op
  return WCI_SUCCESS;
}


uint8_t * 
OCPI::CP289::Worker::
getPropertyVaddr()
{
  return  (uint8_t*)m_rcc_worker->m_context->properties;
}


void 
OCPI::CP289::Worker::
prepareProperty(OCPI::Metadata::Property& md , OCPI::Container::Property& cp)
{
  if (!md.isStruct && !md.members->type.isSequence && md.members->type.scalar != CP::Scalar::OCPI_String &&
      CP::Scalar::sizes[md.members->type.scalar] <= 32 &&
      !md.writeError) {

    if ( (md.offset+sizeof(md.members->type.scalar)) > m_rcc_worker->m_dispatch->propertySize ) {
      throw OCPI::Util::EmbeddedException( PROPERTY_SET_EXCEPTION, NULL, ApplicationRecoverable);
    }
    cp.writeVaddr = (uint8_t*)m_rcc_worker->m_context->properties + md.offset;
  }
}
::WCI_control OCPI::CP289::Worker::wciControls[CM::Worker::OpsLimit] = {
	WCI_CONTROL_INITIALIZE,
	WCI_CONTROL_START,
	WCI_CONTROL_STOP,
	WCI_CONTROL_RELEASE,
	WCI_CONTROL_BEFORE_QUERY,
	WCI_CONTROL_AFTER_CONFIG,
	WCI_CONTROL_RELEASE // for test
};

// We can't user the control mask until the wci stuff is nuked
// And this can be shared better too...
#define CONTROL_OP(x, c, t, s1, s2, s3)                                          \
  void OCPI::CP289::Worker::x() {         					 \
        if (myState == CM::Worker::s1 ||                                         \
            (CM::Worker::s2 != CM::Worker::NONE && myState == CM::Worker::s2) || \
            (CM::Worker::s3 != CM::Worker::NONE && myState == CM::Worker::s3)) { \
	  control(wciControls[CM::Worker::Op##c], WCI_DEFAULT);                  \
          if (CM::Worker::t != CM::Worker::NONE)                                 \
	    myState = CM::Worker::t;					         \
        } else                                                                   \
          throw OCPI::Container::ApiError("Illegal control state for operation",NULL); \
      }
      OCPI_CONTROL_OPS
#undef CONTROL_OP
