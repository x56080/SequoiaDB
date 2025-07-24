/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

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
