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

   Source File Name = catSequenceManager.hpp

   Descriptive Name = Sequence manager

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/19/2018  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CAT_SEQUENCE_MANAGER_HPP_
#define CAT_SEQUENCE_MANAGER_HPP_

#include "oss.hpp"
#include "ossUtil.hpp"
#include "catSequence.hpp"
#include "pmdEDU.hpp"
#include "../bson/bsonobj.h"
#include "utilConcurrentMap.hpp"
#include <string>

namespace engine
{
   struct _catSequenceAcquirer: public SDBObject
   {
      utilSequenceID ID ;
      INT64 nextValue ;
      INT32 acquireSize ;
      INT32 increment ;

      _catSequenceAcquirer()
      {
         ID = UTIL_SEQUENCEID_NULL ;
         nextValue = 0 ;
         acquireSize = 0 ;
         increment = 0 ;
      }
   } ;
   typedef _catSequenceAcquirer catSequenceAcquirer ;

   class _catSequenceManager: public SDBObject
   {
   private:
      typedef utilConcurrentMap<std::string, _catSequence*, 64> CAT_SEQ_MAP ;
      // disallow copy and assign
      _catSequenceManager( const _catSequenceManager& ) ;
      void operator=( const _catSequenceManager& ) ;

   public:
      _catSequenceManager() ;
      ~_catSequenceManager() ;

      INT32 active() ;
      INT32 deactive() ;

      INT32 createSequence( const std::string& name,
                            utilCLUniqueID clUniqueID,
                            const bson::BSONObj& options,
                            _pmdEDUCB* eduCB, INT16 w ) ;
      INT32 insertSequence ( const std::string& name,
                             bson::BSONObj& options, _pmdEDUCB* eduCB,
                             INT16 w ) ;
      INT32 dropSequence( const std::string& name, _pmdEDUCB* eduCB, INT16 w,
                          utilSequenceID* pDroppedSeqID = NULL ) ;
      INT32 alterSequence( const std::string& name,
                           const bson::BSONObj& options,
                           _pmdEDUCB* eduCB,
                           INT16 w,
                           bson::BSONObj* oldOptions,
                           UINT32* alterMask,
                           utilSequenceID* pAlteredSeqID = NULL ) ;
      INT32 renameSequence( const std::string& oldName,
                            const std::string& newName,
                            _pmdEDUCB* eduCB,
                            INT16 w,
                            utilSequenceID* pAlteredSeqID = NULL ) ;
      INT32 acquireSequence( const std::string& name,
                             const utilSequenceID ID,
                             _catSequenceAcquirer& acquirer,
                             _pmdEDUCB* eduCB, INT16 w ) ;
      INT32 resetSequence( const std::string& name, _pmdEDUCB* eduCB, INT16 w ) ;
      INT32 adjustSequence( const std::string& name,
                            const INT64 expectValue,
                            _pmdEDUCB* eduCB, INT16 w,
                            utilSequenceID* pAlteredSeqID = NULL ) ;
      INT32 restartSequence( const std::string& name,
                             const INT64 startValue,
                             _pmdEDUCB* eduCB,
                             INT16 w,
                             utilSequenceID* pAlteredSeqID = NULL ) ;
      OSS_INLINE INT32 getSequence( const std::string& name,
                                    bson::BSONObj& sequence,
                                    _pmdEDUCB* eduCB )
      {
         return _findSequence( name, sequence, eduCB ) ;
      }

   private:
      class _operateSequence ;
      class _acquireSequence ;
      class _adjustSequence ;
      friend class _acquireSequence ;
      friend class _adjustSequence ;

      INT32 _insertSequence( bson::BSONObj& sequence, _pmdEDUCB* eduCB, INT16 w ) ;
      INT32 _deleteSequence( const std::string& name, _pmdEDUCB* eduCB, INT16 w,
                             utilSequenceID* pDeletedSeqID = NULL ) ;
      INT32 _updateSequence( const std::string& name, const bson::BSONObj& options,
                             _pmdEDUCB* eduCB, INT16 w,
                             utilSequenceID* pUpdatedSeqID = NULL ) ;
      INT32 _findSequence( const std::string& name, bson::BSONObj& sequence,
                           _pmdEDUCB* eduCB ) ;
      INT32 _doOnSequence( const std::string& name,
                           const utilSequenceID ID,
                           _pmdEDUCB* eduCB,
                           INT16 w,
                           _operateSequence& func ) ;
      INT32 _doOnSequenceBySLock( const std::string& name,
                                  const utilSequenceID ID,
                                  _pmdEDUCB* eduCB,
                                  INT16 w,
                                  _operateSequence& func,
                                  BOOLEAN& noCache,
                                  utilSequenceID &cachedSeqID ) ;
      INT32 _doOnSequenceByXLock( const std::string& name,
                                  const utilSequenceID ID,
                                  _pmdEDUCB* eduCB,
                                  INT16 w,
                                  _operateSequence& func ) ;
      BOOLEAN _removeCacheByID( const std::string& name, utilSequenceID ID ) ;
      void  _cleanCache( BOOLEAN needFlush ) ;

   private:
      CAT_SEQ_MAP _sequenceCache ;
   } ;
   typedef _catSequenceManager catSequenceManager ;
}

#endif /* CAT_SEQUENCE_MANAGER_HPP_ */

