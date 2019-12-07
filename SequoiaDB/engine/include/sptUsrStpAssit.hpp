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

   Source File Name = sptUsrStpAssit.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_USR_STP_ASSIT_HPP__
#define SPT_USR_STP_ASSIT_HPP__

#include "oss.hpp"
#include "sptRemote.hpp"

namespace engine
{

   /*
      _sptUsrStpAssit define
    */
   class _sptUsrStpAssit : public SDBObject
   {
   public:
      _sptUsrStpAssit() ;
      ~_sptUsrStpAssit() ;

   public:
      INT32 connect( const CHAR *hostName, const CHAR *serviceName ) ;
      INT32 disconnect() ;

      INT32 runCommand( const CHAR *command,
                        const CHAR *argument,
                        CHAR **returnBuffer,
                        INT32 &returnCode,
                        BOOLEAN needResult ) ;

      OSS_INLINE BOOLEAN isConnected()
      {
         return ( 0 != _handle ) ;
      }

   protected:
      ossValuePtr _handle ;
      sptRemote   _remote ;
   } ;

   typedef class _sptUsrStpAssit sptUsrStpAssit ;

}

#endif // SPT_USR_STP_ASSIT_HPP__
