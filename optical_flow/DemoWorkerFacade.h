
#ifndef INCLUDED_DEMO_WORKER_FACADE_H
#define INCLUDED_DEMO_WORKER_FACADE_H

#include "OcpiWorker.h"
#include "OcpiPValue.h"
#include "OcpiProperty.h"
#include "OcpiContainerInterface.h"

#include <string>

/*
  operator<< has to be in the global namespace unless we want to add it
  OCPI::Container::Property
*/

std::ostream& operator<< ( std::ostream& stream,
                           OCPI::Container::Property& property );

namespace Demo
{
  class WorkerFacade
  {
    public:

      WorkerFacade ( const char* msg,
                     OCPI::Container::Application* APP,
                     const char* uri,
                     const char* worker_name,
                     const char* inst_name = 0 );

      ~WorkerFacade ( );

      void print_properties ( );

      void set_properties ( const OCPI::Util::PValue* values );

      void get_properties ( OCPI::Util::PValue* values ) const;

      OCPI::Container::Port& port ( const char* name );

      void start ( )
      {
        d_worker.start ( );
      }

      void stop ( )
      {
        d_worker.stop ( );
      }

      void release ( )
      {
        d_worker.release ( );
      }

      void test ( )
      {
        d_worker.test ( );
      }

      void before_query ( )
      {
        d_worker.beforeQuery ( );
      }

      void after_configure ( )
      {
        d_worker.afterConfigure ( );
      }

      OCPI::Container::Worker& worker ( ) const
      {
        return d_worker;
      }

    private:

      std::string d_msg;

      OCPI::Container::Worker& d_worker;

      std::map<std::string, OCPI::Container::Port*> d_ports;

  }; // End: class WorkerFacade

  OCPI::Container::Interface* get_rcc_interface ( );

  OCPI::Container::Interface* get_hdl_interface ( const char* pci_name );

  inline void print_properties ( Demo::WorkerFacade* facade )
  {
    facade->print_properties ( );
  }

  inline void start ( Demo::WorkerFacade* facade )
  {
    facade->start ( );
  }

  inline void stop ( Demo::WorkerFacade* facade )
  {
    facade->stop ( );
  }

  inline void release ( Demo::WorkerFacade* facade )
  {
    facade->release ( );
  }

  inline void test ( Demo::WorkerFacade* facade )
  {
    facade->test ( );
  }

  inline void before_query ( Demo::WorkerFacade* facade )
  {
    facade->before_query ( );
  }

  inline void after_configure ( Demo::WorkerFacade* facade )
  {
    facade->after_configure ( );
  }

} // End: namespace Demo

#endif // End: #ifndef INCLUDED_DEMO_WORKER_FACADE_H

