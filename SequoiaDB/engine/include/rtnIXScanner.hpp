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

   Source File Name = rtnIXScanner.hpp

   Descriptive Name = RunTime Index Scanner Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for index
   scanner, which is used to traverse index tree.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/09/2018  YXC Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTNIXSCANNER_HPP__
#define RTNIXSCANNER_HPP__

#include "oss.hpp"
#include "dms.hpp"
#include "ixm.hpp"
#include "monCB.hpp"
#include "utilPooledObject.hpp"
#include "rtnPredicate.hpp"
#include "ossMemPool.hpp"
#include "../bson/ordering.h"
#include "../bson/oid.h"
#include "utilPooledAutoPtr.hpp"

using namespace bson ;

namespace engine
{
   class _dmsStorageUnit ;
   class _pmdEDUCB ;

   // define type of index scanners
   enum IXScannerType
   {
      SCANNER_TYPE_DISK      = 0,
      SCANNER_TYPE_MEM_TREE,
      SCANNER_TYPE_MERGE,
      SCANNER_TYPE_MAX
   } ;

   typedef ossPoolSet<dmsRecordID>           SET_RECORDID ;

   /*
      _rtnScannerSharedInfo define
   */
   class _rtnScannerSharedInfo : public SDBObject
   {
   public:
      _rtnScannerSharedInfo() ;
      ~_rtnScannerSharedInfo() ;

      BOOLEAN     insert( const dmsRecordID &rid ) ;
      BOOLEAN     remove( const dmsRecordID &rid ) ;
      BOOLEAN     exists( const dmsRecordID &rid ) const ;

      void        clear() ;

      BOOLEAN     isUpToLimit() const ;
      UINT64      getSize() const ;

   private:
      // need to store a duplicate buffer for dmsRecordID for each ixscan,
      // because each record may be refered by more than one index key (say
      // {c1:[1,2,3,4,5]}, there will be 5 index keys pointing to the same
      // record), so index scan may return the same record more than one time,
      // which should be avoided. Thus we keep a set ( maybe we can further
      // improve performance by customize data structure ) to record id that
      // already scanned.
      // note that a record id will not be changed during update because it will
      // use overflowed record with existing record head
      // we used a pointer here because the memIXTreeScanner are likely used
      // together with another scanner (like disk scanner), we should share
      // the same dupBuffer.
      SET_RECORDID              _setDuplicate ;

   } ;
   typedef _rtnScannerSharedInfo rtnScannerSharedInfo ;

   /*
      _IRtnIXScannerHandler define
    */
   class _IRtnIXScannerHandler
   {
   public:
      _IRtnIXScannerHandler() {}
      virtual ~_IRtnIXScannerHandler() {}

      virtual INT32 onScannerAdvanced( const bson::BSONObj &key, BOOLEAN isRecord ) = 0 ;
   } ;

   typedef class _IRtnIXScannerHandler IRtnIXScannerHandler ;

   /*
      _rtnIXScanner define
      This is a super class for all index scanners to inherit from
   */
   class _rtnIXScanner : public _utilPooledObject
   {
   public:
      _rtnIXScanner( ixmIndexCB *pIndexCB,
                     rtnPredicateList *predList,
                     IRtnIXScannerHandler *pHandler,
                     _dmsStorageUnit  *su,
                     _pmdEDUCB        *cb,
                     BOOLEAN indexCBOwned = FALSE ) ;

      virtual ~_rtnIXScanner() ;

      BOOLEAN     isReadonly() const ;

      void        setShareInfo( rtnScannerSharedInfo *pInfo ) ;
      BOOLEAN     isShareInfoFromSelf() const ;
      BOOLEAN     removeDuplicatRID( const dmsRecordID &rid ) ;

      dmsExtentID getIdxLID() const ;
      const OID&  getIdxOID() const ;
      INT32       getDirection () const ;

      rtnPredicateList*       getPredicateList () ;
      rtnScannerSharedInfo*   getSharedInfo() ;
      IRtnIXScannerHandler*   getScannerHandler()
      {
         return _pHandler ;
      }

      _dmsStorageUnit*        getSu() ;
      ixmIndexCB*             getIndexCB() ;
      BOOLEAN                 getIndexCBOwned() const ;
      _pmdEDUCB*              getEDUCB() ;

      INT32       compareWithCurKeyObj( const BSONObj &keyObj ) const ;
      INT32       syncPredStatus( _rtnIXScanner *source ) ;

      BOOLEAN     eof() const ;
      BOOLEAN                isIndexCover() const ;
      void                   setIndexCover( const BOOLEAN indexCover ) ;
      ixmIndexCover&         getIndex() ;

   /// Interface
   public:
      virtual INT32 init() ;
      virtual void  setReadonly( BOOLEAN isReadonly ) ;

      virtual INT32 advance ( dmsRecordID &rid ) = 0 ;
      virtual INT32 resumeScan( BOOLEAN *pIsCursorSame = NULL ) = 0 ;
      virtual INT32 pauseScan() = 0 ;

      virtual INT32 relocateRID( const BSONObj &keyObj,
                                 const dmsRecordID &rid ) = 0 ;

      virtual BOOLEAN         isAvailable() const = 0 ;
      virtual IXScannerType   getType() const = 0 ;
      virtual IXScannerType   getCurScanType() const = 0 ;
      virtual void            disableByType( IXScannerType type ) = 0 ;
      virtual BOOLEAN         isTypeEnabled( IXScannerType type ) const = 0 ;
      /*
         return : -1, SHARED or EXCLUSIVE
      */
      virtual INT32           getLockModeByType( IXScannerType type ) const = 0 ;

      virtual const BSONObj*  getCurKeyObj() const = 0 ;
      virtual const dmsRecordID& getSavedRID () const = 0 ;
      virtual const BSONObj*  getSavedObj () const = 0 ;

      virtual INT32           isCursorSame( const BSONObj &saveObj,
                                            const dmsRecordID &saveRID,
                                            BOOLEAN &isSame ) = 0 ;

   protected:
      virtual INT32 relocateRID( BOOLEAN &found ) = 0 ;
      virtual rtnPredicateListIterator*   getPredicateListInterator() = 0 ;

   protected:
      BOOLEAN                 _insert2Dup( const dmsRecordID &rid ) ;

   protected:
      ixmIndexCB              *_indexCB ;
      BOOLEAN                 _owned ;
      rtnPredicateList        *_pPredList ;
      _dmsStorageUnit         *_su ;
      _pmdEDUCB               *_cb ;
      rtnScannerSharedInfo    *_pInfo ;

      INT32                   _direction ;
      dmsExtentID             _indexLID ;
      dmsExtentID             _indexCBExtent ;
      OID                     _indexOID ;
      Ordering                _order ;
      BOOLEAN                 _eof ;

      IRtnIXScannerHandler    *_pHandler ;

   private:
      BOOLEAN                 _isReadonly ;
      rtnScannerSharedInfo    _sharedInfo ;
      BOOLEAN                 _indexCover ;
      ixmIndexCover           _index ;
   } ;
   typedef class _rtnIXScanner rtnIXScanner ;

}

#endif //RTNIXSCANNER_HPP__

