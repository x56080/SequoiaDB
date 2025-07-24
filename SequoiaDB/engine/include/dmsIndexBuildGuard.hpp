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

   Source File Name = dmsIndexBuildGuard.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SDB_DMS_INDEX_BUILD_GUARD_HPP_
#define SDB_DMS_INDEX_BUILD_GUARD_HPP_

#include "ossUtil.hpp"
#include "dms.hpp"
#include "ixm.hpp"
#include "ossRWMutex.hpp"
#include "dmsMetadata.hpp"

namespace engine
{

   /*
      _dmsIndexBuildGuard define
    */
   class _dmsIndexBuildGuard : public _utilPooledObject
   {
   public:
      _dmsIndexBuildGuard() = default ;
      ~_dmsIndexBuildGuard() = default ;

      INT32 init( const dmsRecordID &rid ) ;

      // checkers for data writer (user session)
      INT32 writeCheck( const dmsRecordID &rid,
                        BOOLEAN &needProcess,
                        BOOLEAN &hasRisk ) ;
      void writeCommit( const dmsRecordID &rid, BOOLEAN hasRisk ) ;
      void writeAbort( const dmsRecordID &rid, BOOLEAN hasRisk ) ;

      // checkers for index builder
      void buildSetRange( const dmsRecordID &minRID,
                          const dmsRecordID &maxRID ) ;
      INT32 buildCheck( const dmsRecordID &rid,
                        BOOLEAN isBuildByRange,
                        BOOLEAN &needProcess,
                        BOOLEAN &needWait,
                        BOOLEAN &needMove ) ;
      INT32 buildCheckAfterWait( const dmsRecordID &rid,
                                 BOOLEAN &needProcess ) ;
      void buildDone( BOOLEAN isBuildByRange, const dmsRecordID &rid ) ;
      void buildEnd() ;
      void buildExit() ;
      INT32 buildMove( const dmsRecordID &rid ) ;
      INT32 buildCheckMove( const dmsRecordID &id, BOOLEAN &needMove ) ;

      ossRWMutex &getProcessMutex()
      {
         return _processMutex ;
      }

      BOOLEAN isSetByWrite( const dmsRecordID &rid ) ;

   protected:
      ossRWMutex _processMutex ;
      ossSpinXLatch _ridLatch ;
      // current processing record ID range
      dmsRecordID _minRID ;
      dmsRecordID _maxRID ;
      // current scanned record ID range
      // only used when building index by sorting
      dmsRecordID _buildingMinRID ;
      dmsRecordID _buildingMaxRID ;
      // wait event for data writer to wait builder finish processing
      // conflicts records
      ossEvent _buildingEvent ;
      // counts for records greater than processing record ID range
      UINT64 _beyondMaxCount = 0 ;
      // builder is ended
      BOOLEAN _isEnded = FALSE ;
      // key: record ID
      // value: TRUE means processed by builder,
      //        FALSE means processed by data writer(user session)
      ossPoolMap<dmsRecordID, BOOLEAN> _recordMap ;
   } ;

   typedef class _dmsIndexBuildGuard dmsIndexBuildGuard ;
   typedef std::shared_ptr<dmsIndexBuildGuard> dmsIndexBuildGuardPtr ;
   typedef ossPoolMap<dmsIdxMetadataKey, dmsIndexBuildGuardPtr> dmsIdxBuildGuardMap ;
   typedef dmsIdxBuildGuardMap::iterator dmsIdxBuildGuardMapIter ;

}

#endif // SDB_DMS_INDEX_BUILD_GUARD_HPP_
