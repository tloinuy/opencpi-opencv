
#ifndef INCLUDED_OCPI_RPL_DRIVER
#define INCLUDED_OCPI_RPL_DRIVER

#include "OcpiDriver.h"
#include "OcpiUtilException.h"

namespace OCPI
{
  namespace RPL
  {
    class Driver : public OCPI::Util::Driver
    {
      private:

        // The fd for mapped memory, until we have a driver to restrict it.
        static int pciMemFd;

      public:
        // This constructor simply registers itself. This class has no state.
        Driver ( );

        // This driver method is called when container-discovery happens,
        // to see if there are any container devices supported by this driver
        // It uses a generic PCI scanner to find candidates, and when found,
        // calls the "found" method below.
        virtual unsigned int search ( const OCPI::Util::PValue*,
                                      const char** )
        throw ( OCPI::Util::EmbeddedException );

      // This driver method is called to see if a particular container
      // device exists, and if so, to instantiate a container
      virtual OCPI::Util::Device* probe ( const OCPI::Util::PValue*,
                                         const char* which )
      throw ( OCPI::Util::EmbeddedException );

      virtual ~Driver ( )
      throw ( );
    };

  } // End: namespace RPL

} // End: namespace OCPI

#endif // End: #ifndef INCLUDED_OCPI_RPL_DRIVER
