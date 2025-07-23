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

   Source File Name = coordOmStrategyAccessor.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/13/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef COORD_OM_STRATEGY_ACCESSOR_HPP__
#define COORD_OM_STRATEGY_ACCESSOR_HPP__

#include "omStrategyDef.hpp"
#include "pmdEDU.hpp"
#include "rtnContextBuff.hpp"
#include <vector>

namespace engine
{

   class _IOmProxy ;

   /*
      _coordOmStrategyAccessor define
   */
   class _coordOmStrategyAccessor : public SDBObject
   {
      public:
         _coordOmStrategyAccessor( INT64 timeout = -1 ) ;
         ~_coordOmStrategyAccessor() ;

         void  setTimeout( INT64 timeout ) ;

         INT32 getMetaInfoFromOm( omStrategyMetaInfo &metaInfo,
                                  pmdEDUCB *cb,
                                  rtnContextBuf *buf ) ;

         INT32 getStrategyInfoFromOm( vector<omTaskStrategyInfoPtr> &vecInfo,
                                      pmdEDUCB *cb,
                                      rtnContextBuf *buf ) ;

         INT32 getTaskInfoFromOm( vector<omTaskInfoPtr> &vecInfo,
                                  pmdEDUCB *cb,
                                  rtnContextBuf *buf ) ;

      private:
         _IOmProxy               *_pOmProxy ;
         INT64                   _oprTimeout ;

         string                  _clsName ;
         string                  _bizName ;

   } ;
   typedef _coordOmStrategyAccessor coordOmStrategyAccessor ;

}

#endif // COORD_OM_STRATEGY_ACCESSOR_HPP__
