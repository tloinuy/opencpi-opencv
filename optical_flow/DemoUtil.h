
#ifndef INCLUDED_DEMO_UTIL_H
#define INCLUDED_DEMO_UTIL_H

#include "OcpiProperty.h"
#include "OcpiUtilIomanip.h"
#include "OcpiUtilProperty.h"

#include <string>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iostream>
#include <typeinfo>

namespace OCPI
{
  namespace Container
  {
    class Worker;
  }
}

namespace Demo
{
  inline void dispatch ( OCPI::Container::Interface* container )
  {
    container->dispatch ( 0 );
  }

  inline const std::string get_rcc_uri ( const char* worker_name )
  {
    std::stringstream ss;

#if defined ( __PPC__ )
    ss << "/home/mpepe/projects/opencpi/opencpi/components/lib/rcc/linux-MCS_864x/"
       << worker_name
       << ".so";
#elif defined ( __x86_64__ )
		// modified to handle all available RCC workers
    ss << "/home/tonyliu/Desktop/opencpi-opencv/opencpi/components/"
			 << worker_name << ".rcc"
			 << "/target-linux-x86_64/"
       << worker_name
       << ".so";
#elif defined ( __i386__ ) || ( __i686__ )
    ss << "/home/mpepe/projects/opencpi/opencpi/components/lib/rcc/linux-x86_32/"
       << worker_name
       << ".so";
#else
    #error "Unable to determine the correct worker path in get_rcc_uri()"
#endif

    return ss.str ( );
  }

} // End: namespace Demo

#endif // End: #ifndef INCLUDED_DEMO_UTIL_H


