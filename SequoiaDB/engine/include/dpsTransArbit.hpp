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

   Source File Name = dpsTransArbit.hpp

   Descriptive Name = DPS Global Transaction Arbitration

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains declare for arbitration of
   global transactions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/20/2020  HGM  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DPS_TRANS_ARBIT_HPP_
#define DPS_TRANS_ARBIT_HPP_

#include "dpsTransID.hpp"
#include "dpsTransDef.hpp"
#include "ossMemPool.hpp"
#include "ossRWMutex.hpp"
#include "utilPooledObject.hpp"

namespace engine
{

   /*
      _dpsTransArbitRecord define and implement
    */
   // _dpsTransArbitRecord save arbitration record for a single
   // write transaction
   class _dpsTransArbitRecord : public utilPooledObject
   {
   public:
      // constructors and destructor
      _dpsTransArbitRecord()
      : _status( DPS_TRANS_UNKNOWN ),
        _visible( FALSE )
      {
      }

      _dpsTransArbitRecord( DPS_TRANS_STATUS status,
                            BOOLEAN visible )
      : _status( status ),
        _visible( visible )
      {
      }

      _dpsTransArbitRecord( const _dpsTransArbitRecord &record )
      : _status( record._status ),
        _visible( record._visible )
      {
      }

      ~_dpsTransArbitRecord()
      {
      }

   public:
      // assign operator
      OSS_INLINE _dpsTransArbitRecord &operator =(
                                          const _dpsTransArbitRecord &record )
      {
         _status = record._status ;
         _visible = record._visible ;

         return ( *this ) ;
      }

   public:
      // get status
      OSS_INLINE DPS_TRANS_STATUS getStatus() const
      {
         return _status ;
      }

      // check if visible to changes of write transaction
      OSS_INLINE BOOLEAN isVisible() const
      {
         return _visible ;
      }

   protected:
      // status of arbitrate write transaction
      DPS_TRANS_STATUS  _status ;
      // indicate if visible to changes of write transaction
      BOOLEAN           _visible ;
   } ;

   typedef class _dpsTransArbitRecord dpsTransArbitRecord ;
   typedef ossPoolMap< DPS_TRANS_ID,
                       dpsTransArbitRecord >    DPS_ARBIT_RECORD_MAP ;
   typedef DPS_ARBIT_RECORD_MAP::iterator       DPS_ARBIT_RECORD_MAP_IT ;
   typedef DPS_ARBIT_RECORD_MAP::const_iterator DPS_ARBIT_RECORD_MAP_CIT ;

   /*
      _dpsTransArbit define
    */
   // _dpsTransArbit saves all arbitration records for a read transaction
   class _dpsTransArbit : public utilPooledObject
   {
   public:
      // constructor and destructor
      _dpsTransArbit() ;
      ~_dpsTransArbit() ;

   public:
      // arbitrate current transaction against given write transaction
      // input:
      //    - writeTransID: transaction ID of write transaction
      //    - writeTransStatus: transaction status of write transaction
      // output:
      //    - visible: indicate if current transaction could see changes
      //               from write transaction
      // NOTE: only when write transaction is committed, current transaction
      //       could see the changes from write transaction
      // return:
      //    - SDB_OK: succeed to arbitrate
      //    - other errors: failed to arbitrate
      INT32    arbit( const DPS_TRANS_ID &transID,
                      DPS_TRANS_STATUS status,
                      BOOLEAN &visible ) ;

      // find arbitration records for given write transaction
      // input:
      //    - writeTransID: transaction ID of write transaction
      // output:
      //    - visible: indicate if current transaction could see changes
      //               from write transaction
      // return:
      //    - TRUE: record exists ( had been arbitrated before )
      //    - FALSE: record does not exist
      BOOLEAN  findArbit( const DPS_TRANS_ID &transID,
                          BOOLEAN &visible ) ;

      // save arbitration result for given write transaction
      // input:
      //    - transID: transaction ID of write transaction
      //    - status: transaction status of write transaction
      //    - visible: indicate if current transaction could see changes
      //               from write transaction
      // return:
      //    - SDB_OK: succeed to arbitrate
      //    - other errors: failed to arbitrate
      INT32    saveArbit( const DPS_TRANS_ID &transID,
                          DPS_TRANS_STATUS status,
                          BOOLEAN visible ) ;

      // clear arbitration records
      void     clear() ;

   protected:
      // find arbitration records for given write transaction
      // input:
      //    - transID: transaction ID of write transaction
      // output:
      //    - status: status of write transaction
      //    - visible: indicate if current transaction could see changes
      //               from write transaction
      // return:
      //    - TRUE: record exists ( had been arbitrated before )
      //    - FALSE: record does not exist
      // WARNING: should shared acquire mutex
      BOOLEAN  _findRecord( const DPS_TRANS_ID &transID,
                            DPS_TRANS_STATUS &status,
                            BOOLEAN &visible ) ;

      // save arbitration result for given write transaction
      // input:
      //    - transID: transaction ID of write transaction
      //    - status: transaction status of write transaction
      //    - visible: indicate if current transaction could see changes
      //               from write transaction
      // return:
      //    - SDB_OK: succeed to arbitrate
      //    - other errors: failed to arbitrate
      // WARNING: should exclusive acquire mutex
      INT32    _saveRecord( const DPS_TRANS_ID &transID,
                            DPS_TRANS_STATUS status,
                            BOOLEAN visible ) ;

   protected:
      // mutex to protect arbitration record map
      ossRWMutex           _mutex ;
      // map to save arbitration records
      DPS_ARBIT_RECORD_MAP _records ;
   } ;

   typedef class _dpsTransArbit dpsTransArbit ;

}

#endif // DPS_TRANS_ARBIT_HPP_
