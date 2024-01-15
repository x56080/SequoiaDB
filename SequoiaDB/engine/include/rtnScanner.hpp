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

   Source File Name = rtnScanner.hpp

   Descriptive Name = RunTime Scanner Header

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_SCANNER_HPP__
#define RTN_SCANNER_HPP__

#include "oss.hpp"
#include "dms.hpp"
#include "utilPooledObject.hpp"
#include "ossMemPool.hpp"
#include "interface/IStorageSession.hpp"

namespace engine
{

   // forward declaration
   class _dmsStorageUnit ;
   class _dmsMBContext ;
   class _pmdEDUCB ;

   // define type of scanners
   enum rtnScannerType
   {
      SCANNER_TYPE_DISK      = 0,
      SCANNER_TYPE_MEM_TREE,
      SCANNER_TYPE_MERGE
   } ;

   enum rtnScannerStorageType
   {
      SCANNER_TYPE_DATA = 0,
      SCANNER_TYPE_INDEX,
   } ;

   /*
      RTN_SUB_SCAN_TYPE define
   */
   enum RTN_SUB_SCAN_TYPE
   {
      SCAN_NONE,
      SCAN_LEFT,
      SCAN_RIGHT
   } ;

   typedef ossPoolSet<dmsRecordID> SET_RECORDID ;

   /*
      _rtnScanner define
    */
   class _rtnScanner : public _utilPooledObject
   {
   public:
      _rtnScanner( _dmsStorageUnit  *su,
                   _dmsMBContext    *mbContext,
                   INT32             direction,
                   BOOLEAN           isAsync,
                   _pmdEDUCB        *cb )
      : _su( su ),
        _mbContext( mbContext ),
        _direction( direction ),
        _isAsync( isAsync ),
        _isEOF( FALSE ),
        _cb( cb )
      {
      }

      virtual ~_rtnScanner()
      {
      }

   public:
      BOOLEAN isEOF() const
      {
         return _isEOF ;
      }

      virtual INT32 init() = 0 ;
      virtual INT32 advance( dmsRecordID &rid ) = 0 ;
      virtual INT32 resumeScan( BOOLEAN &isCursorSame ) = 0 ;
      virtual INT32 pauseScan() = 0 ;

      virtual INT32 checkSnapshotID( BOOLEAN &isCursorSame ) = 0 ;
      virtual BOOLEAN removeDuplicatRID( const dmsRecordID &rid ) = 0 ;
      virtual dmsExtentID getIdxLID() const = 0 ;
      /*
         return : -1, SHARED or EXCLUSIVE
      */
      virtual INT32 getIdxLockModeByType( rtnScannerType type ) const = 0 ;

      virtual rtnScannerStorageType getStorageType() const = 0 ;
      virtual rtnScannerType  getType() const = 0 ;
      virtual rtnScannerType  getCurScanType() const = 0 ;
      virtual void            disableByType( rtnScannerType type ) = 0 ;
      virtual BOOLEAN         isTypeEnabled( rtnScannerType type ) const = 0 ;
      virtual BOOLEAN         canPrefetch() const = 0 ;
      virtual IStorageSession *getSession() = 0 ;

      _pmdEDUCB *getEDUCB()
      {
         return _cb ;
      }

   protected:
      _dmsStorageUnit *_su ;
      _dmsMBContext *_mbContext ;
      INT32 _direction ;
      BOOLEAN _isAsync ;
      BOOLEAN _isEOF ;
      _pmdEDUCB *_cb ;
   } ;

   typedef class _rtnScanner rtnScanner ;

}

#endif // RTN_SCANNER_HPP__