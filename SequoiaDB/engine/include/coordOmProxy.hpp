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

   Source File Name = coordOmProxy.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          13/03/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef COORD_OM_PROXY_HPP__
#define COORD_OM_PROXY_HPP__

#include "sdbIOmProxy.hpp"
#include "oss.hpp"

using namespace bson ;

namespace engine
{

   class _coordResource ;

   /*
      _coordOmProxy define
   */
   class _coordOmProxy : public _IOmProxy, public SDBObject
   {
      public:
         _coordOmProxy() ;
         virtual ~_coordOmProxy() ;

         INT32          init( _coordResource *pResource ) ;

      public:
         virtual INT32  queryOnOm( MsgHeader *pMsg,
                                  INT32 requestType,
                                  pmdEDUCB *cb,
                                  INT64 &contextID,
                                  rtnContextBuf *buf ) ;

         virtual INT32 queryOnOm( const rtnQueryOptions &options,
                                  pmdEDUCB *cb,
                                  SINT64 &contextID,
                                  rtnContextBuf *buf ) ;

         virtual INT32 queryOnOmAndPushToVec( const rtnQueryOptions &options,
                                              pmdEDUCB *cb,
                                              vector<BSONObj> &objs,
                                              rtnContextBuf *buf ) ;

         virtual void  setOprTimeout( INT64 timeout ) ;

      protected:
         _coordResource                *_pResource ;
         INT64                         _oprTimeout ;

   } ;
   typedef _coordOmProxy coordOmProxy ;

}

#endif //COORD_OM_PROXY_HPP__

