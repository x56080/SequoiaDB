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

   Source File Name = rtnMergeSorting.hpp

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

#ifndef RTNMERGESORTING_HPP_
#define RTNMERGESORTING_HPP_

#include "rtnSortDef.hpp"
#include "dmsTmpBlkUnit.hpp"
#include "rtnSortTuple.hpp"
#include "../bson/ordering.h"

namespace engine
{
   class _pmdEDUCB ;

   class _rtnMergeBlock : public SDBObject
   {
   public:
      _rtnMergeBlock()
      :_buf( NULL ),
       _size( 0 ),
       _read( 0 ),
       _loadSize( 0 ),
       _limit(-1)
      {

      }

      virtual ~_rtnMergeBlock(){}

   public:
      void init( _dmsTmpBlk &blk, CHAR *begin,
                 UINT64 size, SINT64 limit = -1 ) ;

      INT32 next( _dmsTmpBlkUnit *unit, _rtnSortTuple **tuple ) ;

   private:
      INT32 _loadData( _dmsTmpBlkUnit *unit ) ;
   private:
      _dmsTmpBlk _blk ;
      CHAR *_buf ;
      UINT64 _size ;
      UINT64 _read ;
      UINT64 _loadSize ;
      SINT64 _limit ;
   } ;


   class _rtnMergeSorting : public SDBObject
   {
   public:
      _rtnMergeSorting( _dmsTmpBlkUnit *unit,
                        const BSONObj &orderby ) ;
      virtual ~_rtnMergeSorting() ;

   public:
      INT32 init( CHAR *buf, UINT64 size,
                  RTN_SORT_BLKS &src, SINT64 limit = -1) ;

      INT32 fetch( BSONObj &key, const CHAR** obj, INT32* objLen, _pmdEDUCB *cb ) ;

   private:
      INT32 _merge( _pmdEDUCB *cb ) ;

      INT32 _mergeBlks( RTN_SORT_BLKS &src,
                        RTN_SORT_BLKS &dst,
                        _pmdEDUCB *cb ) ;

      INT32 _makeHeap( RTN_SORT_BLKS &src,
                       _pmdEDUCB *cb ) ;

      INT32 _pushObjFromSink( UINT32 i ) ;
   private:
      struct _rtnMergeUnitHelper
      {
         UINT64 _originalSize ;
         UINT64 _newBlkSize ;
         UINT64 _outputStart ;

         _rtnMergeUnitHelper()
         :_originalSize(0),
          _newBlkSize(0),
          _outputStart(0)
         {

         }
      } ;

   private:
      Ordering _order ;
      CHAR *_buf ;
      UINT64 _size ;
      UINT64 _mergeBufSize ;
      UINT64 _mergePos ;
      SINT64 _limit;

      _rtnMergeUnitHelper _unitHelper ;

      _rtnMergeBlock _dataSink[RTN_SORT_MAX_MERGESIZE] ;
      RTN_MERGE_HEAP _heap ;
      UINT32 _mergeBlkSize ;
      UINT32 _mergeMax ;
      RTN_SORT_BLKS _merged ;
      RTN_SORT_BLKS *_src ;

      BOOLEAN _mergeDone ;

      _dmsTmpBlkUnit *_unit ;
   } ;
}

#endif

