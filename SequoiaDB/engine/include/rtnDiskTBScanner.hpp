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

   Source File Name = rtnDiskTBScanner.hpp

   Descriptive Name = RunTime Table Scanner Header

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_DISK_TB_SCANNER_HPP__
#define RTN_DISK_TB_SCANNER_HPP__

#include "rtnTBScanner.hpp"
#include "interface/ICursor.hpp"

namespace engine
{

   /*
      _rtnDiskTBScanner define
    */
   class _rtnDiskTBScanner : public _rtnTBScanner
   {
   public:
      _rtnDiskTBScanner( _dmsStorageUnit  *su,
                         _dmsMBContext    *mbContext,
                         const dmsRecordID &startRID,
                         BOOLEAN           isAfterStartRID,
                         INT32             direction,
                         BOOLEAN           isAsync,
                         _pmdEDUCB        *cb ) ;
      virtual ~_rtnDiskTBScanner() ;

   public:
      // for _rtnScanner
      virtual INT32 advance( dmsRecordID &rid ) ;
      virtual INT32 resumeScan( BOOLEAN &isCursorSame ) ;
      virtual INT32 pauseScan() ;
      virtual INT32 checkSnapshotID( BOOLEAN &isCursorSame ) ;

      virtual rtnScannerType getType() const
      {
         return SCANNER_TYPE_DISK ;
      }

      virtual rtnScannerType getCurScanType() const
      {
         return SCANNER_TYPE_DISK ;
      }

      virtual void disableByType( rtnScannerType type )
      {
      }

      virtual BOOLEAN isTypeEnabled( rtnScannerType type ) const
      {
         return SCANNER_TYPE_DISK == type ;
      }

      // for _rtnTBScanner
      virtual INT32 getCurrentRID( dmsRecordID &nextRID ) ;
      virtual INT32 getCurrentRecord( dmsRecordData &recordData ) ;

      virtual INT32 relocateRID( const dmsRecordID &rid ) ;

      virtual INT32 relocateRID( const dmsRecordID &rid, BOOLEAN &isFound )
      {
         return _relocateRID( rid, isFound ) ;
      }

      virtual BOOLEAN canPrefetch() const
      {
         return _cursorPtr ? _cursorPtr->isAsync() : FALSE ;
      }

      virtual IStorageSession *getSession()
      {
         return _cursorPtr ? _cursorPtr->getSession() : nullptr ;
      }

   protected:
      INT32 _firstInit() ;
      INT32 _relocateRID( const dmsRecordID &rid, BOOLEAN &isFound ) ;

   protected:
      std::unique_ptr<IDataCursor> _cursorPtr ;
   } ;

   typedef class _rtnDiskTBScanner rtnDiskTBScanner ;

}

#endif // RTN_DISK_TB_SCANNER_HPP__