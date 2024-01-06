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

   Source File Name = rtnTBScanner.hpp

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
#ifndef RTN_TB_SCANNER_HPP__
#define RTN_TB_SCANNER_HPP__

#include "rtnScanner.hpp"
#include "interface/ICursor.hpp"

namespace engine
{

   /*
      _rtnTBScanner define
    */
   class _rtnTBScanner : public _rtnScanner
   {
   public:
      _rtnTBScanner( _dmsStorageUnit  *su,
                     _dmsMBContext    *mbContext,
                     const dmsRecordID &startRID,
                     BOOLEAN           isAfterStartRID,
                     INT32             direction,
                     _pmdEDUCB        *cb )
      : _rtnScanner( su, mbContext, direction, cb ),
        _init( FALSE ),
        _startRID( startRID ),
        _isAfterStartRID( isAfterStartRID )
      {
      }

      virtual ~_rtnTBScanner() = default ;

      virtual rtnScannerStorageType getStorageType() const
      {
         return SCANNER_TYPE_DATA ;
      }

      virtual INT32 getIdxLockModeByType( rtnScannerType type ) const
      {
         return -1 ;
      }

      virtual BOOLEAN removeDuplicatRID( const dmsRecordID &rid )
      {
         return TRUE ;
      }

      virtual dmsExtentID getIdxLID() const
      {
         return DMS_INVALID_EXTENT ;
      }

   public:
      BOOLEAN isEOF() const
      {
         return _isEOF ;
      }

      virtual INT32 getCurrentRID( dmsRecordID &nextRID ) = 0 ;
      virtual INT32 getCurrentRecord( dmsRecordData &recordData ) = 0 ;

      virtual INT32 relocateRID( const dmsRecordID &rid ) = 0 ;
      virtual INT32 relocateRID( const dmsRecordID &rid, BOOLEAN &isFound ) = 0 ;

      const dmsRecordID &getSavedRID() const
      {
         return _savedRID ;
      }

      void resetSavedRID()
      {
         _savedRID.reset() ;
         _relocatedRID.reset() ;
      }

      BOOLEAN isInit() const
      {
         return _init ;
      }

   protected:
      BOOLEAN _init ;
      dmsRecordID _startRID ;
      BOOLEAN _isAfterStartRID ;
      dmsRecordID _savedRID ;
      dmsRecordID _relocatedRID ;
   } ;

   typedef class _rtnTBScanner rtnTBScanner ;

}

#endif // RTN_TB_SCANNER_HPP__
