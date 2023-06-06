/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = sdbmkgen.cpp

   Descriptive Name = SequoiaDB Master Key Generator

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains code logic for
   data dump and integrity check.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/23/2023  ZHY Initial Draft

   Last Changed =

*******************************************************************************/

#include "ossGMCrypto.hpp"
#include "ossVer.h"
#include "openssl/err.h"
#include <iostream>
#include <boost/program_options.hpp>

using namespace std ;

boost::program_options::variables_map vm ;
boost::program_options::options_description desc ;

struct options
{
   std::string file ;
} OPTIONS ;

INT32 parseCommands( int argc, char **argv )
{
   using namespace boost::program_options ;
   INT32 rc = SDB_OK ;
   try
   {
      desc.add_options()( "help,h", "help info" )(
         "file,f", value< std::string >( &OPTIONS.file ),
         "output file path" )( "version,v", "version" ) ;

      store( parse_command_line( argc, argv, desc ), vm ) ;
      notify( vm ) ;
   }
   catch ( unknown_option &e )
   {
      std::cerr <<  "Unknown argument: " << e.get_option_name () << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   catch ( invalid_option_value &e )
   {
      std::cerr <<  "Invalid argument: "
                << e.get_option_name () << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   catch( error &e )
   {
      std::cerr << e.what () << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

int main( int argc, char **argv )
{
   INT32 rc = SDB_OK ;
   EVP_PKEY *pkey = NULL ;
   rc = parseCommands( argc, argv ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   if ( vm.count( "help" ) )
   {
      std::cout << desc << std::endl ;
      goto done ;
   }

   if ( vm.count( "version" ) )
   {
      ossPrintVersion( "SDB Master Key Generator" ) ;
      rc = SDB_PMD_VERSION_ONLY ;
      goto done ;
   }

   if ( OPTIONS.file.empty() )
   {
      std::cerr << "Must specify the output file" << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = ossSM2GenKeyPair( &pkey ) ;
   if ( SDB_OK != rc )
   {
      std::cerr << "Failed to generate SM2 key pair" << std::endl ;
      goto error ;
   }

   rc = ossSM2WriteKeyPairToFile( OPTIONS.file.c_str(), pkey ) ;
   if ( SDB_OK != rc )
   {
      INT32 code = ERR_get_error();
      CHAR msg[256];
      ERR_error_string_n(code, msg, sizeof(msg));
      std::cerr << "Failed to write SM2 key pair to file, reason: " << msg
                << std::endl ;
      goto error ;
   }

done:
   ossSM2FreeKeyPair( pkey ) ;
   return rc ;
error:
   goto done ;
}