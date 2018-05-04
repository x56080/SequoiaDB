/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = clsCatalogCaller.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Replication component. This file contains structure for
   replication control block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLSCATALOGCALLER_HPP_
#define CLSCATALOGCALLER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "msg.hpp"

#include <map>

using namespace std ;
namespace engine
{
   class _netRouteAgent ;

   struct _clsCataCallerMeta
   {
      MsgHeader *header ;
      UINT32 bufLen ;
      INT32  timeout ;
      UINT32 sendTimes ;

      _clsCataCallerMeta()
      :header(NULL),
       bufLen(0),
       timeout(-1),
       sendTimes(1)
      {
      }

      ~_clsCataCallerMeta()
      {
         if ( NULL != header )
         {
            SDB_OSS_FREE( header ) ;
            header = NULL ;
         }
         bufLen = 0 ;
         timeout = -1 ;
         sendTimes = 0 ;
      }
   } ;
   typedef std::map<UINT32, _clsCataCallerMeta> callerMeta ;

   class _clsCatalogCaller : public SDBObject
   {
   public:
      _clsCatalogCaller() ;
      ~_clsCatalogCaller() ;

   public:
      INT32 call( MsgHeader *header, UINT32 times = 1 ) ;

      void remove( const MsgHeader *header, INT32 result ) ;
      void remove( INT32 opCode ) ;

      void handleTimeout( const UINT32 &millisec ) ;

   private:
      callerMeta _meta ;

   } ;

   typedef class _clsCatalogCaller clsCatalogCaller ;
}

#endif

