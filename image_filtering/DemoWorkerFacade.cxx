
#include "DemoWorkerFacade.h"

#include "OcpiRplDriver.h"
#include "OcpiUtilIomanip.h"
#include "OcpiUtilProperty.h"
#include "OcpiUtilException.h"

#include <iomanip>
#include <iostream>

namespace
{
  OCPI::Util::DriverManager* get_hdl_driver_manager ( )
  {
    static OCPI::RPL::Driver* driver ( 0 );
    static OCPI::Util::DriverManager* dm ( 0 );
#if 0

    if ( !driver )
    {
      // Memory leak
      // When declared static the program crashes after main() exits.
      driver = new OCPI::RPL::Driver ( );

      dm = new OCPI::Util::DriverManager ( "OCFRP" );

      dm->discoverDevices ( 0, 0 );

      return dm;
    }
#endif

    return dm;
  }

  OCPI::Util::DriverManager* get_rcc_driver_manager ( )
  {
    OCPI::Util::DriverManager* dm ( 0 );

    if ( !dm )
    {
      // Another memory leak
      // When declared static the program crashes after main() exits.
      dm = new OCPI::Util::DriverManager ( "Container" );

      dm->discoverDevices ( 0, 0 );
    }

    return dm;
  }

} // End: namespace<unamed>

/* ---- Find an HDL container -------------------------------------------- */

OCPI::Container::Interface* Demo::get_hdl_interface ( const char* name )
{
  OCPI::Container::Interface* interface =
                        static_cast<OCPI::Container::Interface*>
                        ( get_hdl_driver_manager( )->getDevice ( 0, name ) );

  return interface;
}

/* ---- Find an RCC container -------------------------------------------- */

OCPI::Container::Interface* Demo::get_rcc_interface ( )
{
  OCPI::Util::PValue props [ 2 ] = { OCPI::Util::PVBool ( "polling", 1 ),
                                    OCPI::Util::PVEnd };

  OCPI::Util::Device* d =
                    get_rcc_driver_manager ( )->getDevice ( props, "RCC" );

  if ( !d )
  {
    throw OCPI::Util::EmbeddedException ( "No RCC containers found.\n" );
  }

  return static_cast<OCPI::Container::Interface*> ( d );
}

/* ---- Function to print OCPI::Container::Property objects --------------- */

/*
  operator<< has to be in the global namespace unless we want to add it
  OCPI::Container::Property
*/

std::ostream& operator<< ( std::ostream& stream,
                           OCPI::Container::Property& property )
{
  stream << "\n  "
         << std::setw ( 32 )
         << std::setfill ( ' ' )
         << std::left
         << property.getName ( );

  if ( !property.isReadable ( ) )
  {
    stream << "property is write-only";
  }
  else // Property is readable
  {
    switch ( property.getType ( ) )
    {
      case OCPI::Util::Prop::Scalar::OCPI_Bool:
        stream << std::boolalpha
               << property.getBoolValue ( );
        break;
      case OCPI::Util::Prop::Scalar::OCPI_Char:
        stream << OCPI::Util::Iomanip::hex ( 8 )
               << property.getCharValue ( );
        break;
      case OCPI::Util::Prop::Scalar::OCPI_UChar:
        stream << OCPI::Util::Iomanip::hex ( 8 )
               << property.getUCharValue ( );
        break;
      case OCPI::Util::Prop::Scalar::OCPI_Short:
        stream << OCPI::Util::Iomanip::hex ( 8 )
               << property.getShortValue ( );
        break;
      case OCPI::Util::Prop::Scalar::OCPI_UShort:
        stream << OCPI::Util::Iomanip::hex ( 8 )
               << property.getUShortValue ( );
        break;
      case OCPI::Util::Prop::Scalar::OCPI_Long:
        stream << OCPI::Util::Iomanip::hex ( 8 )
               << property.getLongValue ( );
        break;
      case OCPI::Util::Prop::Scalar::OCPI_ULong:
        stream << OCPI::Util::Iomanip::hex ( 8 )
               << property.getULongValue ( );
        break;
      case OCPI::Util::Prop::Scalar::OCPI_LongLong:
        stream << OCPI::Util::Iomanip::hex ( 16 )
               << property.getLongLongValue ( );
        break;
      case OCPI::Util::Prop::Scalar::OCPI_ULongLong:
        stream << OCPI::Util::Iomanip::hex ( 16 )
               << property.getULongLongValue ( );
        break;
      case OCPI::Util::Prop::Scalar::OCPI_Double:
        stream << OCPI::Util::Iomanip::flt ( 16, 8 )
               << property.getDoubleValue ( );
        break;
      case OCPI::Util::Prop::Scalar::OCPI_Float:
        stream << OCPI::Util::Iomanip::flt ( 16, 8 )
               << property.getFloatValue ( );
        break;
      case OCPI::Util::Prop::Scalar::OCPI_String:
        {
          char value [ 257 ];
          property.getStringValue ( value, sizeof ( value ) );
          stream << value;
        }
        break;
      default:
        stream << "error : unknown property type : "
               << property.getType ( );
        break;
    }
  }

  return stream;
}

/* ---- Implementation of Demo::WorkerFacade ------------------------ */

namespace
{
  void get_all_ports ( OCPI::Container::Worker& worker,
                       std::map<std::string, OCPI::Container::Port*>& ports )
  {
    unsigned int n_ports ( 0 );

    OCPI::Metadata::Port* p = worker.getPorts ( n_ports );

    for ( std::size_t n = 0; n < n_ports; n++ )
    {
      ports [ p [ n ].name ] = &( worker.getPort ( p [ n ].name ) );
    }
  }

} // End: namespace<unamed>

Demo::WorkerFacade::WorkerFacade ( const char* msg,
                                        OCPI::Container::Application* app,
                                        const char* uri,
                                        const char* worker_name,
                                        const char* inst_name )
  : d_msg ( msg ),
    d_worker ( app->createWorker ( uri,
                                   0,
                                   worker_name,
                                   inst_name ) ),
    d_ports ( )
{
  get_all_ports ( d_worker, d_ports );
}

Demo::WorkerFacade::~WorkerFacade ( )
{
  // Empty
}

void Demo::WorkerFacade::print_properties ( )
{
  unsigned int n_props ( 0 );

  OCPI::Metadata::Property* p ( d_worker.getProperties ( n_props ) );

  std::cout << "\n\n"
            << d_msg
            << " worker properties";

  for ( std::size_t n = 0; n < n_props; n++ )
  {
    {
      OCPI::Container::Property prop ( d_worker, p [ n ].name );

      std::cout << prop;
    }
  }

  std::cout << std::endl;
}

void Demo::WorkerFacade::set_properties ( const OCPI::Util::PValue* values )
{
  for ( std::size_t n = 0; values && values [ n ].name; n++ )
  {
    OCPI::Container::Property property ( d_worker, values [ n ].name );

    if ( !property.isWritable ( ) )
    {
      std::cerr << "Property named "
                << values [ n ].name
                << " is read-only."
                << std::endl;
      continue;
    }

    switch ( property.getType ( ) )
    {
      case OCPI::Util::Prop::Scalar::OCPI_Bool:
        property.setBoolValue ( values [ n ].vBool );
        break;
      case OCPI::Util::Prop::Scalar::OCPI_Char:
        property.setCharValue ( values [ n ].vChar );
        break;
      case OCPI::Util::Prop::Scalar::OCPI_UChar:
        property.setUCharValue ( values [ n ].vUChar );
        break;
      case OCPI::Util::Prop::Scalar::OCPI_Short:
        property.setShortValue ( values [ n ].vShort );
        break;
      case OCPI::Util::Prop::Scalar::OCPI_UShort:
        property.setUShortValue ( values [ n ].vUShort );
        break;
      case OCPI::Util::Prop::Scalar::OCPI_Long:
        property.setLongValue ( values [ n ].vLong );
        break;
      case OCPI::Util::Prop::Scalar::OCPI_ULong:
        property.setULongValue ( values [ n ].vULong );
        break;
      case OCPI::Util::Prop::Scalar::OCPI_LongLong:
        property.setLongLongValue ( values [ n ].vLongLong );
        break;
      case OCPI::Util::Prop::Scalar::OCPI_ULongLong:
        property.setULongLongValue ( values [ n ].vULongLong );
        break;
      case OCPI::Util::Prop::Scalar::OCPI_Double:
        property.setDoubleValue ( values [ n ].vDouble );
        break;
      case OCPI::Util::Prop::Scalar::OCPI_Float:
        property.setFloatValue ( values [ n ].vFloat );
        break;
      case OCPI::Util::Prop::Scalar::OCPI_String:
        property.setStringValue ( values [ n ].vString );
        break;
      default:
        std::cerr << "error : unknown property type : "
                  << property.getType ( )
                  << std::endl;
        break;

    } // End: switch ( property.getType ( ) )

  } // End: for ( std::size_t n = 0; values && values [ n ].name; ...
}

void Demo::WorkerFacade::get_properties ( OCPI::Util::PValue* values ) const
{
  for ( std::size_t n = 0; values && values [ n ].name; n++ )
  {
    OCPI::Container::Property property ( d_worker, values [ n ].name );

    if ( !property.isReadable ( ) )
    {
      std::cerr << "Property named "
                << values [ n ].name
                << " is write-only."
                << std::endl;
      continue;
    }

    switch ( property.getType ( ) )
    {
      case OCPI::Util::Prop::Scalar::OCPI_Bool:
        values [ n ].vBool = property.getBoolValue ( );
        break;
      case OCPI::Util::Prop::Scalar::OCPI_Char:
        values [ n ].vChar = property.getCharValue ( );
        break;
      case OCPI::Util::Prop::Scalar::OCPI_UChar:
        values [ n ].vUChar = property.getUCharValue ( );
        break;
      case OCPI::Util::Prop::Scalar::OCPI_Short:
        values [ n ].vShort = property.getShortValue ( );
        break;
      case OCPI::Util::Prop::Scalar::OCPI_UShort:
        values [ n ].vUShort = property.getUShortValue ( );
        break;
      case OCPI::Util::Prop::Scalar::OCPI_Long:
        values [ n ].vLong = property.getLongValue ( );
        break;
      case OCPI::Util::Prop::Scalar::OCPI_ULong:
        values [ n ].vULong = property.getULongValue ( );
        break;
      case OCPI::Util::Prop::Scalar::OCPI_LongLong:
        values [ n ].vLongLong = property.getLongLongValue ( );
        break;
      case OCPI::Util::Prop::Scalar::OCPI_ULongLong:
        values [ n ].vULongLong = property.getULongLongValue ( );
        break;
      case OCPI::Util::Prop::Scalar::OCPI_Double:
        values [ n ].vDouble = property.getDoubleValue ( );
        break;
      case OCPI::Util::Prop::Scalar::OCPI_Float:
        values [ n ].vFloat = property.getFloatValue ( );
        break;
      case OCPI::Util::Prop::Scalar::OCPI_String:
        property.getStringValue ( values [ n ].vString,
                                  sizeof ( values [ n ].vString ) );
        break;
      default:
        std::cerr << "error : unknown property type : "
                  << property.getType ( )
                  << std::endl;
        break;

    } // End: switch ( property.getType ( ) )

  } // End: for ( std::size_t n = 0; values && values [ n ].name; ...
}


OCPI::Container::Port& Demo::WorkerFacade::port ( const char* name )
{
  if ( d_ports [ name ] == 0 )
  {
    d_ports [ name ] = &( d_worker.getPort ( name ) );
  }

  return *( d_ports [ name ] );
}

