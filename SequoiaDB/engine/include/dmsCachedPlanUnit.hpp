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

   Source File Name = dmsCachedPlanUnit.hpp

   Descriptive Name = DMS Cached Access Plan Units Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   management of cached access plans of collections.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/10/2017  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMSCACHEDPLANUNIT_HPP__
#define DMSCACHEDPLANUNIT_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "dms.hpp"
#include "utilSUCache.hpp"
#include "utilHashTable.hpp"
#include "../bson/bson.h"

using namespace bson ;

namespace engine
{

   #define DMS_PARAM_INVALID_THRESHOLD ( 5 )

   /*
      _dmsCLCachedPlanUnit define
    */
   class _dmsCLCachedPlanUnit : public _utilSUCacheUnit
   {
      public :
         _dmsCLCachedPlanUnit () ;

         _dmsCLCachedPlanUnit ( UINT16 mbID, UINT64 createTime ) ;

         virtual ~_dmsCLCachedPlanUnit () ;

         virtual UINT8 getUnitType () const
         {
            return UTIL_SU_CACHE_UNIT_CLPLAN ;
         }

         OSS_INLINE virtual BOOLEAN addSubUnit ( utilSUCacheUnit *pSubUnit,
                                                 BOOLEAN ignoreCrtTime )
         {
            return FALSE ;
         }

         OSS_INLINE virtual void clearSubUnits () {}

         OSS_INLINE void incParamInvalid ()
         {
            _paramInvalidCount ++ ;
         }

         OSS_INLINE BOOLEAN isParamInvalid () const
         {
            return _paramInvalidCount > DMS_PARAM_INVALID_THRESHOLD ;
         }

         OSS_INLINE void incMainCLInvalid ()
         {
            _mainCLInvalidCount ++ ;
         }

         OSS_INLINE void setMainCLInvalid ()
         {
            _mainCLInvalidCount = DMS_PARAM_INVALID_THRESHOLD + 1 ;
         }

         OSS_INLINE BOOLEAN isMainCLInvalid ()
         {
            return _mainCLInvalidCount > DMS_PARAM_INVALID_THRESHOLD ;
         }

      protected :
         UINT8    _paramInvalidCount ;
         UINT8    _mainCLInvalidCount ;
   } ;

   typedef class _dmsCLCachedPlanUnit dmsCLCachedPlanUnit ;

}

#endif //DMSCACHEDPLANUNIT_HPP__

