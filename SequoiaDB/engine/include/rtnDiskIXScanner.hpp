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

   Source File Name = rtnDiskIXScanner.hpp

   Descriptive Name = RunTime Disk Index Scanner Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for index
   scanner, which is used to traverse index tree.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTNDISKIXSCANNER_HPP__
#define RTNDISKIXSCANNER_HPP__

#include "rtnIXScanner.hpp"
#include "interface/ICursor.hpp"

using namespace bson ;

namespace engine
{

   /*
      _rtnDiskIXScanner define
      A scanner to traverse through on disk index pages/slots
   */
   class _rtnDiskIXScanner : public _rtnIXScanner
   {
   public:
      _rtnDiskIXScanner ( ixmIndexCB *pIndexCB,
                          rtnPredicateList *predList,
                          _dmsStorageUnit  *su,
                          _dmsMBContext    *mbContext,
                          BOOLEAN          isAsync,
                          _pmdEDUCB        *cb,
                          BOOLEAN indexCBOwnned = FALSE ) ;

      virtual ~_rtnDiskIXScanner () ;

   /// Interface
   public:
      virtual INT32 advance ( dmsRecordID &rid ) ;
      virtual INT32 resumeScan( BOOLEAN &isCursorSame ) ;
      virtual INT32 pauseScan() ;
      virtual INT32 checkSnapshotID( BOOLEAN &isCursorSame ) ;

      virtual void  setMonCtxCB( _monContextCB *monCtxCB ) ;

      virtual INT32 relocateRID( const BSONObj &keyObj,
                                 const dmsRecordID &rid ) ;

      virtual BOOLEAN         isAvailable() const ;
      virtual rtnScannerType  getType() const ;
      virtual rtnScannerType  getCurScanType() const ;
      virtual void            disableByType( rtnScannerType type ) ;
      virtual BOOLEAN         isTypeEnabled( rtnScannerType type ) const ;
      virtual INT32           getIdxLockModeByType( rtnScannerType type ) const ;

      virtual const BSONObj*  getCurKeyObj() const { return &_curKeyObj ; }
      virtual const dmsRecordID& getSavedRID () const { return _savedRID ; }
      virtual const BSONObj*  getSavedObj () const { return &_savedObj ; }

      virtual BOOLEAN canPrefetch() const
      {
         return _cursorPtr ? _cursorPtr->isAsync() : FALSE ;
      }

   protected:
      virtual INT32 _relocateRID( BOOLEAN &found ) ;
      virtual rtnPredicateListIterator*   _getPredicateListInterator() ;

   protected:
      void                    _reset() ;

      INT32                   _relocateRID( const BSONObj &keyObj,
                                            const dmsRecordID &rid,
                                            INT32 direction,
                                            BOOLEAN &isFound ) ;

      INT32                   _firstInit() ;
      INT32                   _advance() ;
      INT32                   _fetchNext( dmsRecordID &rid, BOOLEAN &needAdvance ) ;

   private:
      rtnPredicateListIterator   _listIterator ;
      monContextCB               *_pMonCtxCB ;

      // Flag to indicate if we need to locate/relocate the key
      BOOLEAN                    _init ;

      // after the caller release X latch, another session may change the index
      // structure. So everytime before releasing X latch, the caller must store
      // the current obj, and then verify if it's still remaining the same after
      // next check. If the object doens't match the on-disk obj, something must
      // goes changed and we should relocate
      BSONObj                  _savedObj ;
      dmsRecordID              _savedRID ;

      BSONObj                  _curKeyObj ;
      dmsRecordID              _relocatedRID ;

      BufBuilder               _builder ;

      std::unique_ptr<IIndexCursor> _cursorPtr ;
   } ;
   typedef class _rtnDiskIXScanner rtnDiskIXScanner ;

}

#endif //RTNDISKIXSCANNER_HPP__

