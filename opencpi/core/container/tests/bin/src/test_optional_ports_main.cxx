
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
 * Coverage test for optional ports
 *
 * 06/10/09 - John Miller
 * Initial Version
 *
 */

#include <stdio.h>
#include <sstream>
#include <stdlib.h>
#include <OcpiOsMisc.h>
#include <OcpiOsAssert.h>
#include <DtIntEventHandler.h>
#include <OcpiTransportServer.h>
#include <OcpiTransportClient.h>
#include <OcpiRDTInterface.h>
#include <test_utilities.h>
#include <OcpiUtilCommandLineConfiguration.h>
#include "UtGenericLoopbackWorkers.h"
#include <OcpiThread.h>

using namespace OCPI::DataTransport;
using namespace DataTransport::Interface;
using namespace OCPI::Container;
using namespace OCPI;
using namespace OCPI::CONTAINER_TEST;

static const char* g_ep1    = "ocpi-smb-pio://test1:900000.1.20";
static const char* g_ep2    = "ocpi-smb-pio://test2:900000.2.20";
static const char* g_ep3    = "ocpi-smb-pio://test3:900000.3.20";
static int   OCPI_USE_POLLING            = 1;

static CWorker PRODUCER(0,3), LOOPBACK(2,3), CONSUMER(4,0);

#define PRODUCER_OUTPUT_PORT0  PORT_0

#define CONSUMER_INPUT_PORT0   PORT_0
#define CONSUMER_INPUT_PORT1   PORT_1
#define CONSUMER_INPUT_PORT2   PORT_2

#define LOOPBACK_INPUT_PORT0   PORT_0
#define LOOPBACK_INPUT_PORT1   PORT_1
#define LOOPBACK_OUTPUT_PORT0  PORT_2
#define LOOPBACK_OUTPUT_PORT1  PORT_3
#define LOOPBACK_OUTPUT_PORT2  PORT_4


class OcpiRccBinderConfigurator
  : public OCPI::Util::CommandLineConfiguration
{
public:
  OcpiRccBinderConfigurator ();

public:
  bool help;
  bool verbose;
  MultiString  endpoints;

private:
  static CommandLineConfiguration::Option g_options[];
};

// Configuration
static  OcpiRccBinderConfigurator config;

OcpiRccBinderConfigurator::
OcpiRccBinderConfigurator ()
  : OCPI::Util::CommandLineConfiguration (g_options),
    help (false),
    verbose (false)
{
}

OCPI::Util::CommandLineConfiguration::Option
OcpiRccBinderConfigurator::g_options[] = {

  { OCPI::Util::CommandLineConfiguration::OptionType::MULTISTRING,
    "endpoints", "container endpoints",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::endpoints), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "verbose", "Be verbose",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::verbose), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::NONE,
    "help", "This message",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::help), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::END, 0, 0, 0, 0 }
};

static
void
printUsage (OcpiRccBinderConfigurator & config,
            const char * argv0)
{
  std::cout << "usage: " << argv0 << " [options]" << std::endl
            << "  options: " << std::endl;
  config.printOptions (std::cout);
}


static void createWorkers(std::vector<CApp>& ca )
{
  try {
    PRODUCER.worker = &ca[PRODUCER.cid].app->createWorker( NULL,NULL, (char *)&UTGProducerWorkerDispatchTable );
    LOOPBACK.worker = &ca[LOOPBACK.cid].app->createWorker( NULL,NULL, (char *)&UTGLoopbackWorkerDispatchTable );
    CONSUMER.worker = &ca[CONSUMER.cid].app->createWorker( NULL,NULL, (char *)&UTGConsumerWorkerDispatchTable );
  }
  CATCH_ALL_RETHROW( "creating workers" )
    }



#define BUFFERS_2_PROCESS 200;
static void initWorkerProperties( std::vector<CApp>& ca )
{
  ( void ) ca;
  WCI_error wcie;
  int32_t  tprop[5], offset, nBytes;

  // Set the producer buffer run count property to 0
  offset = offsetof(UTGProducerWorkerProperties,run2BufferCount);
  nBytes = sizeof( uint32_t );
  tprop[0] = BUFFERS_2_PROCESS;
  wcie =  PRODUCER.worker->write(  offset,
                                              nBytes, WCI_DATA_TYPE_U32, WCI_DEFAULT, &tprop[0]);
  CHECK_WCI_WRITE_ERROR( wcie, ca, PRODUCER);
  wcie =  PRODUCER.worker->control(  WCI_CONTROL_AFTER_CONFIG, WCI_DEFAULT );
  CHECK_WCI_CONROL_ERROR( wcie, WCI_CONTROL_AFTER_CONFIG, ca, PRODUCER);

  // Set the producer buffers processed count
  offset = offsetof(UTGProducerWorkerProperties,buffersProcessed);
  nBytes = sizeof( uint32_t );
  tprop[0] = 0;
  wcie =  PRODUCER.worker->write(  offset,
                                              nBytes, WCI_DATA_TYPE_U32, WCI_DEFAULT, &tprop[0]);
  CHECK_WCI_WRITE_ERROR( wcie, ca, PRODUCER);
  wcie =  PRODUCER.worker->control(  WCI_CONTROL_AFTER_CONFIG, WCI_DEFAULT );
  CHECK_WCI_CONROL_ERROR( wcie, WCI_CONTROL_AFTER_CONFIG, ca, PRODUCER);

  // Set the producer bytes processed count
  offset = offsetof(UTGProducerWorkerProperties,bytesProcessed);
  nBytes = sizeof( uint32_t );
  tprop[0] = 0;
  wcie =  PRODUCER.worker->write(  offset,
                                              nBytes, WCI_DATA_TYPE_U32, WCI_DEFAULT, &tprop[0]);
  CHECK_WCI_WRITE_ERROR( wcie, ca, PRODUCER);
  wcie =  PRODUCER.worker->control(  WCI_CONTROL_AFTER_CONFIG, WCI_DEFAULT );
  CHECK_WCI_CONROL_ERROR( wcie, WCI_CONTROL_AFTER_CONFIG, ca, PRODUCER);

  // Set the consumer passfail property to passed
  offset = offsetof(UTGConsumerWorkerProperties,passfail);
  nBytes = sizeof( uint32_t );
  tprop[0] = 1;
  wcie =  CONSUMER.worker->write(  offset,
                                              nBytes, WCI_DATA_TYPE_U32, WCI_DEFAULT, &tprop[0]);
  CHECK_WCI_WRITE_ERROR( wcie, ca, CONSUMER);
  wcie = CONSUMER.worker->control(  WCI_CONTROL_AFTER_CONFIG, WCI_DEFAULT );
  CHECK_WCI_CONROL_ERROR( wcie, WCI_CONTROL_AFTER_CONFIG, ca, CONSUMER);

  // Set the consumer dropped buffers count
  offset = offsetof(UTGConsumerWorkerProperties,droppedBuffers);
  nBytes = sizeof( uint32_t );
  tprop[0] = 0;
  wcie = CONSUMER.worker->write(  offset,
                                             nBytes, WCI_DATA_TYPE_U32, WCI_DEFAULT, &tprop[0]);
  CHECK_WCI_WRITE_ERROR( wcie, ca, CONSUMER);
  wcie = CONSUMER.worker->control(  WCI_CONTROL_AFTER_CONFIG, WCI_DEFAULT );
  CHECK_WCI_CONROL_ERROR( wcie, WCI_CONTROL_AFTER_CONFIG, ca, CONSUMER);

  // Set the consumer buffer run count property to 0
  offset = offsetof(UTGConsumerWorkerProperties,run2BufferCount);
  nBytes = sizeof( uint32_t );
  tprop[0] = BUFFERS_2_PROCESS;
  wcie = CONSUMER.worker->write(  offset,
                                             nBytes, WCI_DATA_TYPE_U32, WCI_DEFAULT, &tprop[0]);
  CHECK_WCI_WRITE_ERROR( wcie, ca, CONSUMER);
  wcie = CONSUMER.worker->control(  WCI_CONTROL_AFTER_CONFIG, WCI_DEFAULT );
  CHECK_WCI_CONROL_ERROR( wcie, WCI_CONTROL_AFTER_CONFIG, ca, CONSUMER);

  // Set the consumer buffers processed count
  offset = offsetof(UTGConsumerWorkerProperties,buffersProcessed);
  nBytes = sizeof( uint32_t );
  tprop[0] = 0;
  wcie = CONSUMER.worker->write(  offset,
                                             nBytes, WCI_DATA_TYPE_U32, WCI_DEFAULT, &tprop[0]);
  CHECK_WCI_WRITE_ERROR( wcie, ca, CONSUMER);
  wcie = CONSUMER.worker->control(  WCI_CONTROL_AFTER_CONFIG, WCI_DEFAULT );
  CHECK_WCI_CONROL_ERROR( wcie, WCI_CONTROL_AFTER_CONFIG, ca, CONSUMER);

  // Set the consumer buffers processed count
  offset = offsetof(UTGConsumerWorkerProperties,bytesProcessed);
  nBytes = sizeof( uint32_t );
  tprop[0] = 0;
  wcie = CONSUMER.worker->write(  offset,
                                             nBytes, WCI_DATA_TYPE_U32, WCI_DEFAULT, &tprop[0]);
  CHECK_WCI_WRITE_ERROR( wcie, ca, CONSUMER);
  wcie = CONSUMER.worker->control(  WCI_CONTROL_AFTER_CONFIG, WCI_DEFAULT );
  CHECK_WCI_CONROL_ERROR( wcie, WCI_CONTROL_AFTER_CONFIG, ca, CONSUMER);

}



static bool run_lb_test(std::vector<CApp>& ca, std::vector<CWorker*>& workers )
{
  bool passed = true;

  try {
    initWorkerProperties(ca);
  }
  catch( ... ) {
    printf("Failed to init worker properties\n");
    throw;
  }

  try {
    enableWorkers(ca, workers);
  }
  catch( ... ) {
    printf("Failed to enable workers\n");
    throw;
  }

  UTGConsumerWorkerProperties cprops;
  UTGProducerWorkerProperties pprops;

  int count = 6;
  while ( count > 0 ) {

    // Read the consumer properties to monitor progress
    CONSUMER.worker->read(   0,
                                        sizeof(UTGConsumerWorkerProperties),
                                        WCI_DATA_TYPE_U8, WCI_DEFAULT, &cprops);

    if ( cprops.buffersProcessed == cprops.run2BufferCount  ) {

      if ( cprops.droppedBuffers ) {
        printf("\nConsumer dropped %d buffers\n", cprops.droppedBuffers );
        passed = false;
        break;
      }

      // Make sure that the consumer got the same data
      PRODUCER.worker->read(   0,
                                          sizeof(UTGProducerWorkerProperties),
                                          WCI_DATA_TYPE_U8, WCI_DEFAULT, &pprops);

      if ( cprops.bytesProcessed != pprops.bytesProcessed ) {
        printf("Producer produced %d bytes of data, consumer got %d bytes of data\n",
               pprops.bytesProcessed, cprops.bytesProcessed );
        passed = false;
        break;
      }
      else {
        break;
      }

    }
    OCPI::OS::sleep( 1000 );
    count--;
  }

  if ( count == 0 ) {
    printf("\nTest timed out\n");
    passed = false;
  }

  if ( ! passed ) {
    PRODUCER.worker->read(   0,
                                        sizeof(UTGProducerWorkerProperties),
                                        WCI_DATA_TYPE_U8, WCI_DEFAULT, &pprops);
    printf("\nTest failed results:\n");
    printf("   Producer produced %d buffers, consumer received %d buffers\n",
           pprops.buffersProcessed, cprops.buffersProcessed );
  }

  disableWorkers(ca, workers );
  return passed;
}


int config_and_run_optports_test(const char *test_name, std::vector<CApp>& ca,
                                 std::vector<CWorker*>& workers,
                                 int cmap[], int bcmap[] )
{
  ( void ) test_name;
  char tnamebuf[256];
  sprintf(tnamebuf, "Optional ports TEST: container map %d,%d,%d buffer map %d,%d,%d,%d",
          cmap[0], cmap[1], cmap[2], bcmap[0], bcmap[1], bcmap[2], bcmap[3] );

  PRODUCER = cmap[0];
  PRODUCER.pdata[PRODUCER_OUTPUT_PORT0].bufferCount = bcmap[0];
  LOOPBACK = cmap[1];
  LOOPBACK.pdata[LOOPBACK_INPUT_PORT0].bufferCount  = bcmap[1];
  LOOPBACK.pdata[LOOPBACK_OUTPUT_PORT0].bufferCount = bcmap[2];
  CONSUMER = cmap[2];
  CONSUMER.pdata[CONSUMER_INPUT_PORT0].bufferCount  = bcmap[3];

  bool test_rc;
  int testPassed = 1;

  try {
    createWorkers( ca );
  }
  catch ( ... ) {
    printf("Failed to create workers\n");
    throw;
  }

  try {
    createPorts( ca, workers );
  }
  catch ( ... ) {
    printf("Failed to create ports\n");
    throw;
  }

  try {
    connectWorkers( ca, workers);
  }
  catch ( ... ) {
    printf("Failed to connect worker ports\n");
    throw;
  }


  try {
    initWorkers( ca, workers );
  }
  catch( ... ) {
    printf("Failed to init workers\n");
    throw;
  }

  test_rc = run_lb_test(ca, workers );
  if ( test_rc == false ) testPassed = 0;

  disconnectPorts( ca, workers );
  destroyWorkers( ca, workers );

  return testPassed;
}

static int bcmap[][4] = {
  { 1,1,1,1 },
  { 2,2,2,2 },
  { 1,2,3,4 },
  { 4,5,10,1 },
  { 4,5,1,14 },
  { 1,1,10,5 },
  { 10,1,1,5 },
  { 1,10,10,15 },
  { 15,1,1,1 }
};

#ifdef NO_MAIN
int test_optional_ports_main( int argc, char** argv)
#else
int  main( int argc, char** argv)
#endif
{
  int test_rc = 1;
  int oa_test_rc = 1;
  DataTransfer::EventManager* event_manager;
  int cmap[3];
  const char* test_name;

  try {
    config.configure (argc, argv);
  }
  catch (const std::string & oops) {
    std::cerr << "Error: " << oops << std::endl;
    return false;
  }
  if (config.help) {
    printUsage (config, argv[0]);
    return false;
  }
  g_testUtilVerbose = config.verbose;



  cmap[0] = cmap[1] = cmap[2] = 0;

  std::vector<const char*> endpoints;
  endpoints.push_back( g_ep1 );
  endpoints.push_back( g_ep2 );
  endpoints.push_back( g_ep3 );

  std::vector<CApp> ca;

  try {
    ca =
      createContainers(endpoints, event_manager, (bool)OCPI_USE_POLLING);
  }
  catch( std::string& err ) {
    printf("Got a string exception while creating containers = %s\n", err.c_str() );
    exit( -1 );
  }
  catch( OCPI::Util::EmbeddedException& ex ) {
    printf("Create containers failed with exception. errorno = %d, aux = %s\n",
           ex.getErrorCode(), ex.getAuxInfo() );
    exit(-1);
  }
  catch( ... ) {
    printf("Got an unknown exception while creating containers\n");
    exit( -1 );
  }

  // Create a dispatch thread
  DThreadData tdata;
  tdata.run =1;
  tdata.containers = ca;
  tdata.event_manager = event_manager;

  OCPI::Util::Thread* t = runTestDispatch(tdata);

  std::vector<CWorker*> workers;
  workers.push_back( &PRODUCER );
  workers.push_back( &CONSUMER );
  workers.push_back( &LOOPBACK );


  // run the test with port 0 connected and make sure it works
  test_name = "Required port connected";

  // Patch the consumer dispatch table to make sure we have a NULL run-condition
  RCCRunCondition *trc = UTGConsumerWorkerDispatchTable.runCondition;
  UTGConsumerWorkerDispatchTable.runCondition = NULL;
  try {

    // Setup connection info
    PRODUCER = cmap[0];
    PRODUCER.pdata[PRODUCER_OUTPUT_PORT0].down_stream_connection.worker = &LOOPBACK;
    PRODUCER.pdata[PRODUCER_OUTPUT_PORT0].down_stream_connection.pid = LOOPBACK_INPUT_PORT0;

    LOOPBACK = cmap[1];
    LOOPBACK.pdata[LOOPBACK_OUTPUT_PORT0].down_stream_connection.worker = &CONSUMER;
    LOOPBACK.pdata[LOOPBACK_OUTPUT_PORT0].down_stream_connection.pid = CONSUMER_INPUT_PORT0;

    CONSUMER = cmap[2];

    printf("\n\nRunning test (%s): \n", test_name );
    test_rc &= config_and_run_optports_test( test_name, ca, workers, cmap, bcmap[3]);
  }
  catch( OCPI::Util::EmbeddedException& ex ) {
    printf("failed with an exception. errorno = %d, aux = %s\n",
           ex.getErrorCode(), ex.getAuxInfo() );
    test_rc = 0;
  }
  catch ( std::string& str ) {
    printf(" failed with an exception %s\n",
           str.c_str() );
    test_rc = 0;
  }
  catch ( ... ) {
    test_rc = 0;
  }
  UTGConsumerWorkerDispatchTable.runCondition = trc;
  printf(" Test:  %s\n",   test_rc ? "PASSED" : "FAILED" );
  oa_test_rc &= test_rc; test_rc=1;


  // run the test with only port 1 connected and make sure it fails
  test_name = "Required port NOT connected";
  try {
    // Setup connection info
    PRODUCER = 0;
    PRODUCER.pdata[PRODUCER_OUTPUT_PORT0].down_stream_connection.worker = &LOOPBACK;
    PRODUCER.pdata[PRODUCER_OUTPUT_PORT0].down_stream_connection.pid = LOOPBACK_INPUT_PORT0;

    LOOPBACK = 0;
    LOOPBACK.pdata[LOOPBACK_OUTPUT_PORT0].down_stream_connection.worker = &CONSUMER;
    LOOPBACK.pdata[LOOPBACK_OUTPUT_PORT0].down_stream_connection.pid = CONSUMER_INPUT_PORT1;

    CONSUMER = 0;

    printf("\n\nRunning test (%s): \n", test_name );
    test_rc &= config_and_run_optports_test( test_name, ca, workers, cmap, bcmap[3]);
  }
  catch( OCPI::Util::EmbeddedException& ex ) {
    printf("failed with an exception. errorno = %d, aux = %s (NOT EXPECTED)",
           ex.getErrorCode(), ex.getAuxInfo() );
  }
  catch ( std::string& str ) {
    printf("got exception ""%s"", (EXPECTED)", str.c_str() );
    test_rc = 0;
  }
  catch ( ... ) {
    printf("failed with an unknown exception (NOT EXPECTED)\n");
  }

  if ( test_rc == 0 ) {
    printf("\n Test:  %s\n",  "PASSED"  );
    test_rc = 1;
  }
  else {
    printf("\n Test:  %s ",  "FAILED"  );
    printf(" because it did not enforce the optional port mask\n");
    test_rc = 0;
  }
  oa_test_rc &= test_rc; test_rc=1;


  // run the test with port 0 connected and make sure it works
  test_name = "Required port connected plus non-required ports";

  // Patch the consumer dispatch table to make sure we have a NULL run-condition
  trc = UTGConsumerWorkerDispatchTable.runCondition;
  UTGConsumerWorkerDispatchTable.runCondition = NULL;
  try {

    // Setup connection info
    PRODUCER = 0;
    PRODUCER.pdata[PRODUCER_OUTPUT_PORT0].down_stream_connection.worker = &LOOPBACK;
    PRODUCER.pdata[PRODUCER_OUTPUT_PORT0].down_stream_connection.pid = LOOPBACK_INPUT_PORT0;

    LOOPBACK = 0;
    LOOPBACK.pdata[LOOPBACK_OUTPUT_PORT0].down_stream_connection.worker = &CONSUMER;
    LOOPBACK.pdata[LOOPBACK_OUTPUT_PORT0].down_stream_connection.pid = CONSUMER_INPUT_PORT0;

    LOOPBACK.pdata[LOOPBACK_OUTPUT_PORT1].down_stream_connection.worker = &CONSUMER;
    LOOPBACK.pdata[LOOPBACK_OUTPUT_PORT1].down_stream_connection.pid = CONSUMER_INPUT_PORT1;

    CONSUMER = 0;

    printf("\n\nRunning test (%s): \n", test_name );
    test_rc &= config_and_run_optports_test( test_name, ca, workers, cmap, bcmap[3]);
  }
  catch( OCPI::Util::EmbeddedException& ex ) {
    printf("failed with an exception. errorno = %d, aux = %s\n",
           ex.getErrorCode(), ex.getAuxInfo() );
    test_rc = 0;
  }
  catch ( std::string& str ) {
    printf(" failed with an exception %s\n",
           str.c_str() );
    test_rc = 0;
  }
  catch ( ... ) {
    test_rc = 0;
  }
  UTGConsumerWorkerDispatchTable.runCondition = trc;
  printf(" Test:  %s\n",   test_rc ? "PASSED" : "FAILED" );
  oa_test_rc &= test_rc; test_rc=1;


  // run the test with port 0 connected and make sure it works
  test_name = "Required port NOT connected plus multiple non-required ports";

  try {
    // Setup connection info
    PRODUCER = 0;
    PRODUCER.pdata[PRODUCER_OUTPUT_PORT0].down_stream_connection.worker = &LOOPBACK;
    PRODUCER.pdata[PRODUCER_OUTPUT_PORT0].down_stream_connection.pid = LOOPBACK_INPUT_PORT0;

    LOOPBACK = 0;
    LOOPBACK.pdata[LOOPBACK_OUTPUT_PORT0].down_stream_connection.worker = &CONSUMER;
    LOOPBACK.pdata[LOOPBACK_OUTPUT_PORT0].down_stream_connection.pid = CONSUMER_INPUT_PORT1;

    LOOPBACK.pdata[LOOPBACK_OUTPUT_PORT1].down_stream_connection.worker = &CONSUMER;
    LOOPBACK.pdata[LOOPBACK_OUTPUT_PORT1].down_stream_connection.pid = CONSUMER_INPUT_PORT2;

    CONSUMER = 0;

    printf("\n\nRunning test (%s): \n", test_name );
    test_rc &= config_and_run_optports_test( test_name, ca, workers, cmap, bcmap[3]);
  }
  catch( OCPI::Util::EmbeddedException& ex ) {
    printf("failed with an exception. errorno = %d, aux = %s (NOT EXPECTED)",
           ex.getErrorCode(), ex.getAuxInfo() );
  }
  catch ( std::string& str ) {
    printf("got exception ""%s"", (EXPECTED)", str.c_str() );
    test_rc = 0;
  }
  catch ( ... ) {
    printf("failed with an unknown exception (NOT EXPECTED)\n");
  }

  if ( test_rc == 0 ) {
    printf("\n Test:  %s\n",  "PASSED"  );
    test_rc = 1;
  }
  else {
    printf("\n Test:  %s ",  "FAILED"  );
    printf(" because it did not enforce the optional port mask\n");
    test_rc = 0;
  }
  oa_test_rc &= test_rc; test_rc=1;


  tdata.run=0;
  t->join();
  destroyContainers( ca, workers );

  return !oa_test_rc;
}




