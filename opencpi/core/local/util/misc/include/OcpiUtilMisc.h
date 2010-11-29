
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

// -*- c++ -*-

#ifndef OCPIUTILMISC_H__
#define OCPIUTILMISC_H__

/**
 * \file
 * \brief Miscellaneous utilities.
 *
 * Revision History:
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 */

#include <string>
#include <istream>
#include <functional>


#ifndef NDEBUG
  class Trace {
  public:
    std::string ts;
    Trace(const char* e) 
      : ts(e)
    {
      printf("Entering (%s)\n", ts.c_str() );
    }

    ~Trace()
    {
      printf("Leaving (%s)\n", ts.c_str() );    
    }
  };
#define TRACE(s) Trace t(s)
#else
#define TRACE(s)
#endif


namespace OCPI {
  namespace Util {
    /**
     * \brief Miscellaneous utilities.
     */

    namespace Misc {

      /**
       * \brief Convert an integer to a string.
       *
       * \param[in] value An integer value.
       * \return  A string containing the decimal representation of the value.
       */

      std::string integerToString (int value);

      /**
       * \brief Convert an unsigned integer to a string.
       *
       * \param[in] value An unsigned integer value.
       * \param[in] base  The base for representing \a value.  Must be between
       *                  2 and 16, inclusive. E.g., 10 is for decimal, 16 for
       *                  hexadecimal.
       * \param[in] mindigits The minimum of digits that the string
       *                  representation shall have.  If \a value can be
       *                  represented with fewer digits, it is left-padded
       *                  using the \a pad character.
       * \param[in] pad   The character to pad the string with, if necessary.
       * \return          A string representation of \a value, using base
       *                  \a base, having a length of at least \a mindigits.
       */

      std::string unsignedToString (unsigned int value,
                                    unsigned int base = 10,
                                    unsigned int mindigits = 0,
                                    char pad = '0');

      /**
       * \brief Convert an unsigned long long integer to a string.
       *
       * \param[in] value An unsigned integer value.
       * \param[in] base  The base for representing \a value.  Must be between
       *                  2 and 16, inclusive. E.g., 10 is for decimal, 16 for
       *                  hexadecimal.
       * \param[in] mindigits The minimum of digits that the string
       *                  representation shall have.  If \a value can be
       *                  represented with fewer digits, it is left-padded
       *                  using the \a pad character.
       * \param[in] pad   The character to pad the string with, if necessary.
       * \return          A string representation of \a value, using base
       *                  \a base, having a length of at least \a mindigits.
       */

      std::string unsignedToString (unsigned long long value,
                                    unsigned int base = 10,
                                    unsigned int mindigits = 0,
                                    char pad = '0');

      /**
       * \brief Convert a floating-point value to a string.
       *
       * \param[in] value A floating-point value.
       * \return          A decimal representation of the value.
       */

      std::string doubleToString (double value);

      /**
       * \brief Convert a string to an integer.
       *
       * \param[in] value A string containing a decimal representation of
       *                  an integer number.
       * \return          The value.
       *
       * \throw std::string If the value can not be interpreted as a
       * number.
       */

      int stringToInteger (const std::string & value);

      /**
       * \brief Convert a string to an unsigned integer.
       *
       * \param[in] value A string containing a decimal representation
       *                  of an unsigned integer number.
       * \param[in] base  The base.  Must be between 2 and 16, inclusive.
       * \return          The value.
       *
       * \throw std::string If the value can not be interpreted as a
       * number.
       */

      unsigned int stringToUnsigned (const std::string & value,
                                     unsigned int base = 10);

      /**
       * \brief Convert a string to an unsigned integer.
       *
       * \param[in] value A string containing a decimal representation
       *                  of an unsigned integer number.
       * \param[in] base  The base.  Must be between 2 and 16, inclusive.
       * \return          The value.
       *
       * \throw std::string If the value can not be interpreted as a
       * number.
       */

      unsigned long long stringToULongLong (const std::string & value,
                                            unsigned int base = 10);

      /**
       * \brief Convert an unsigned long long position to a <em>std::streamsize</em> value.
       *
       * <em>std::streamsize</em> is an implementation-defined,
       * signed type that may be of lesser width than unsigned long long.
       * This function provides control over the return value if the
       * unsigned long long position value exceeds the value range of
       * <em>std::streamsize</em>.
       *
       * \param[in] pos      The position to convert.
       * \param[in] minusone What to do if the \a pos value exceeds the value
       *                range of <em>std::streamsize</em>.  If true, then -1
       *                will be returned as an error indicator.  If false,
       *                then the maximum positive value for
       *                <em>std::streamsize</em> is returned.
       * \return        An <em>std::streamsize</em> value with the same
       *                value as \a pos, if possible, or -1 or the maximum
       *                positive value for <em>std::streamsize</em>,
       *                as requested using the \a minusone parameter.
       */

      std::streamsize unsignedToStreamsize (unsigned long long pos,
                                            bool minusone = true);

      /**
       * \brief Read a line from an input stream to the next LF.
       *
       * Read a line from an input stream, up to and including the next
       * <tt>LF</tt> character.  Trailing whitespace characters (including
       * the <tt>LF</tt> and any <tt>CR</tt>, tab and space characters) are
       * discarded.
       *
       * Up to \a maxSize characters are read.  If no end-of-line character
       * is found after reading \a maxSize characters, the \a failbit is set
       * on \a conn, and the data that was read up to that point is returned.
       *
       * \param[in] conn    The input stream from which to read.
       * \param[in] maxSize The maximum amount of characters to read, or
       *                    -1 for no maximum.
       * \return            The data that was read.
       */

      std::string readline (std::istream * conn, unsigned int maxSize = static_cast<unsigned int> (-1));

      /**
       * \brief Case insensitive string comparison.
       *
       * \param[in] s1  The first string.
       * \param[in] s2  The second string.
       * \return   The result of a case-insensitive comparison of \a s1 and
       *           \a s2.  A negative value if \a s1 is lexicographically less
       *           than \a s2.  A positive value if \a s1 is lexicographically
       *           greater than \a s2.  Zero if the strings are the same,
       *           excluding case.
       */

      int caseInsensitiveStringCompare (const std::string & s1,
                                        const std::string & s2);

      /**
       * \brief Predicate to compare two strings, ignoring case.
       *
       * This class can be used, e.g., wherever the STL expects a
       * comparator class, as in the <em>std::set</em> and <em>std::map</em>
       * templates.  E.g.,
       *
       * \code
       *   typedef std::map<std::string, std::string,
       *                    CaseInsensitiveStringLess>
       *     CaseInsensitiveMap;
       * \endcode
       */

      class CaseInsensitiveStringLess : public std::binary_function<std::string, std::string, bool>
      {
      public:
        bool operator() (const std::string & s1, const std::string & s2) const;
      };

      /**
       * \brief Test if a string matches a glob-style pattern.
       *
       * \param[in] str     A string.
       * \param[in] pattern A glob-style pattern.
       * \return            true if \a str matches \a pattern,
       *                    false if not.
       */

      bool glob (const std::string & str, const std::string & pattern);

      /**
       * \brief Determine if a file is an XML document.
       *
       * This operation reads data from the stream.  After reaching a
       * conclusion, the stream pointer is rewound to the beginning.
       * If resetting the read pointer fails, e.g., if the stream is
       * not seekable, the caller may have to close and re-open the
       * file before processing it.
       *
       * \param[in] str A stream.
       * \return   true if the data in the stream looks like an XML
       *           document, false if not.
       *
       * \pre The read pointer shall be at position zero.
       * \post The read pointer is at position zero, or the failbit is
       * set on \a str (because repositioning the read pointer failed).
       */

      bool isXMLDocument (std::istream * str);

    }
  }
}

#endif
