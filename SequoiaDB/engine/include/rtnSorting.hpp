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

   Source File Name = rtnSorting.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains declare for runtime
   functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTNSORTING_HPP_
#define RTNSORTING_HPP_

#include "rtnSortDef.hpp"
#include "rtnMergeSorting.hpp"
#include "dmsTmpBlkUnit.hpp"
#include "utilCommBuff.hpp"

using namespace bson ;

namespace engine
{
   class _pmdEDUCB ;
   class _rtnInternalSorting ;


   class _rtnSorting : public SDBObject
   {
   public:
      _rtnSorting();
      virtual ~_rtnSorting();

   public:
      INT32 init( UINT64 bufSize,
                  const BSONObj &orderby,
                  SINT64 fino,
                  SINT64 limit,
                  _pmdEDUCB *cb );

      INT32 push( const BSONObj& key, const CHAR* obj, INT32 objLen,
                  BSONElement* arrEle, _pmdEDUCB *cb ) ;

      INT32 sort( _pmdEDUCB *cb ) ;

      /// do not ensure that the key and obj is get owned.
      INT32 fetch( BSONObj &key, const CHAR** obj,
                   INT32* objLen, _pmdEDUCB *cb ) ;

      OSS_INLINE BOOLEAN isInMemorySort () const
      {
         return _blks.empty() ;
      }

   private:

      INT32 _moveToExternalBlks( _rtnInternalSorting *inter,
                                 RTN_SORT_BLKS &blks,
                                 _pmdEDUCB *cb ) ;

      INT32 _fetchFromInter( BSONObj &key, const CHAR** obj, INT32* objLen ) ;

      INT32 _fetchFromExter( BSONObj &next, const CHAR** obj, INT32* objLen,
                             _pmdEDUCB *cb ) ;
   private:
      _dmsTmpBlkUnit _unit ;
      BSONObj _orderby ;

      // Buffer used by internal/merge sorting.
      utilBuffMonitor _buffMonitor ;
      utilCommBuff *_bucketBuff ; // For internal sorting only.
      utilCommBuff *_objBuff ;

      RTN_SORT_STEP _step ;
      _pmdEDUCB *_cb ;
      _rtnInternalSorting *_internalBlk ;
      _rtnMergeSorting *_mergeBlk ;
      RTN_SORT_BLKS _blks ;
      UINT64 _blkBegin ;
      SINT64 _fino ;
      SINT64 _limit ;
      CHAR *_cpBuf ;
      const UINT32 _cpBufSize ;
   };

   typedef class _rtnSorting rtnSorting;
}

#endif

