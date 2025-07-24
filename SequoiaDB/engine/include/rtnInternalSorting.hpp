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

   Source File Name = rtnInternalSorting.hpp

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
#ifndef RTNINTERNALSORTING_HPP_
#define RTNINTERNALSORTING_HPP_

#include "rtnSortDef.hpp"
#include "rtnSortTuple.hpp"
#include "ixmIndexKey.hpp"
#include "../bson/ordering.h"
#include "utilCommBuff.hpp"

namespace engine
{
   class _pmdEDUCB ;

   /**
    * Internal sorting implementation.
    * It will use several blocks of memory, allocated dynamically by sort area.
    * This is to avoid allocating large fixed size memory from the beginning,
    * which will results in memory waste.
    */
   class _rtnInternalSorting : public utilPooledObject
   {
   public:
      _rtnInternalSorting( const BSONObj &orderby,
                           utilCommBuff *tupleDirectory,
                           utilCommBuff *tupleBuff,
                           INT64 limit ) ;
      virtual ~_rtnInternalSorting() ;

   public:
      INT32 push( const BSONObj& keyObj, const CHAR* obj,
                  INT32 objLen, BSONElement* arrElement ) ;

      void clearBuf() ;

      INT32 sort( _pmdEDUCB *cb ) ;

      BOOLEAN more() const
      {
         if ( _limit > 0 && _fetched >= (UINT64)_limit )
         {
            return FALSE ;
         }
         return  _fetched < _objNum ;
      }

      INT32 next( _rtnSortTuple **tuple ) ;

      UINT64 getObjNum () { return _objNum ; }

      UINT32 maxRecordSize() const
      {
         return _maxRecordSize ;
      }

   private:
      INT32 _quickSort( _rtnSortTuple **left,
                        _rtnSortTuple **right,
                        _pmdEDUCB *cb ) ;

      INT32 _partition( _rtnSortTuple **left,
                        _rtnSortTuple **right,
                        _rtnSortTuple **&leftAxis,
                        _rtnSortTuple **&rightAxis ) ;

      INT32 _insertSort( _rtnSortTuple **left,
                         _rtnSortTuple **right ) ;

      INT32 _swapLeftSameKey( _rtnSortTuple **left,
                              _rtnSortTuple **right,
                              _rtnSortTuple **&axis ) ;

      INT32 _swapRightSameKey( _rtnSortTuple **left,
                               _rtnSortTuple **right,
                               _rtnSortTuple **&axis ) ;

   private:
      bson::Ordering _order ;
      utilCommBuff *_tupleDirectory ;
      utilCommBuff *_tupleBuff ;

      UINT64 _objNum ;
      UINT64 _fetched ;
      UINT64 _recursion ;
      INT64  _limit ;
      UINT32 _maxRecordSize ;
   } ;
}

#endif

