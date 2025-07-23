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

   Source File Name = dpsMergeBlock.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DPSMERGEBLOCK_H_
#define DPSMERGEBLOCK_H_

#include "core.hpp"
#include "oss.hpp"
#include "dpsLogDef.hpp"
#include "dpsLogRecord.hpp"
#include "dpsPageMeta.hpp"
#include "dms.hpp"
#include "sdbInterface.hpp"
#include "utilDataExInfo.hpp"

namespace engine
{
   class _dpsLogPage ;
   class _dpsReplicaLogMgr ;
   class _dpsMergeInfo ;

   /*
      _dpsMergeBlock define
   */
   class _dpsMergeBlock : public SDBObject
   {
      friend class _dpsReplicaLogMgr ;
      friend class _dpsMergeInfo ;

      private:
         dpsLogRecord _record ;
         dpsPageMeta _pageMeta ;
         BOOLEAN _isRow;

      public:
         _dpsMergeBlock();

         ~_dpsMergeBlock();

      public:
         OSS_INLINE BOOLEAN isRow()
         {
            return _isRow;
         }

         OSS_INLINE void setRow( BOOLEAN isRow )
         {
            _isRow = isRow;
         }

         OSS_INLINE dpsLogRecord &record()
         {
            return _record ;
         }

         OSS_INLINE dpsPageMeta &pageMeta()
         {
            return _pageMeta ;
         }

      public:
         void clear();
   };

   typedef class _dpsMergeBlock dpsMergeBlock;

   /*
      _dpsMergeInfo define
   */
   class _dpsMergeInfo : public _utilLogExInfo
   {
      public:
         _dpsMergeInfo () ;
         _dpsMergeInfo ( dpsMergeBlock &block ) ;
         ~_dpsMergeInfo () ;

         dpsMergeBlock &getDummyBlock () ;
         dpsMergeBlock &getMergeBlock () ;
         BOOLEAN       hasDummy () ;
         void clear()
         {
            _mergeBlock.clear() ;
            _dummyBlock.clear() ;
            _hasDummy = FALSE ;
         }
         void setInfoEx( IExecutor *cb,
                         utilCSUniqueID csUID = UTIL_UNIQUEID_NULL,
                         UINT32 csLID = ~0,
                         utilCLUniqueID clUID = UTIL_UNIQUEID_NULL,
                         UINT32 clLID = ~0,
                         dmsExtentID extLID = DMS_INVALID_EXTENT )
         {
            _setInfo( csUID, csLID, clUID, clLID, extLID ) ;
            _pCB     = cb ;
         }
         void setDefInfoEx( IExecutor *cb = NULL )
         {
            _setInfo( UTIL_UNIQUEID_NULL, ~0, UTIL_UNIQUEID_NULL, ~0, DMS_INVALID_EXTENT ) ;
            _pCB     = cb ;
         }
         void enableTrans()
         {
            _transEnabled = TRUE ;
         }
         void resetInfoEx()
         {
            _resetInfo() ;
            _transEnabled = FALSE ;
            _pCB     = NULL ;
         }
         BOOLEAN isNeedNotify() const { return isValid() ; }
         BOOLEAN isTransEnabled() const { return _transEnabled ; }
         IExecutor* getEDUCB() const { return _pCB ; }

         void disableCache()
         {
            _cache.release() ;
            _isCacheEnabled = FALSE ;
         }

         BOOLEAN isCacheEnabled() const
         {
            return _isCacheEnabled ;
         }

      private:
         dpsMergeBlock        _mergeBlock ;
         dpsMergeBlock        _dummyBlock ;
         dpsMergeBlock        &_refer ;
         BOOLEAN              _hasDummy ;

         BOOLEAN              _transEnabled ;
         IExecutor            *_pCB ;

         BOOLEAN              _isCacheEnabled ;
   } ;

   typedef class _dpsMergeInfo dpsMergeInfo ;

}

#endif
