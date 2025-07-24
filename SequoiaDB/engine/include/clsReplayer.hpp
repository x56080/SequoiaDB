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

   Source File Name = clsReplayer.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime code for insert
   request.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CLSREPLAYER_HPP_
#define CLSREPLAYER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "dpsLogRecord.hpp"
#include "rtnBackgroundJob.hpp"
#include "utilCompressor.hpp"
#include "../bson/bsonobj.h"

using namespace bson ;

namespace engine
{
   class _pmdEDUCB ;
   class _SDB_DMSCB ;
   class _dpsLogWrapper ;
   class _clsBucket ;

   /*
      _clsReplayer define
   */
   class _clsReplayer : public SDBObject
   {
   public:
      _clsReplayer( BOOLEAN useDps = FALSE, BOOLEAN isReplSync = FALSE ) ;
      ~_clsReplayer() ;

      void enableDPS () ;
      void disableDPS () ;
      BOOLEAN isDPSEnabled () const { return _dpsCB ? TRUE : FALSE ; }

   public:
      INT32 replay( dpsLogRecordHeader *recordHeader, _pmdEDUCB *eduCB,
                    BOOLEAN incCount = TRUE ) ;
      INT32 replayByBucket( dpsLogRecordHeader *recordHeader,
                            _pmdEDUCB *eduCB, _clsBucket *pBucket ) ;

      INT32 replayCrtCS( const CHAR *cs, INT32 pageSize, INT32 lobPageSize,
                         _pmdEDUCB *eduCB ) ;

      INT32 replayCrtCollection( const CHAR *collection,
                                 UINT32 attributes,
                                 _pmdEDUCB *eduCB,
                                 UTIL_COMPRESSOR_TYPE compType ) ;

      INT32 replayIXCrt( const CHAR *collection,
                         BSONObj &index,
                         _pmdEDUCB *eduCB ) ;

      INT32 replayInsert( const CHAR *collection,
                          BSONObj &obj,
                          _pmdEDUCB *eduCB ) ;

      INT32 rollback( const dpsLogRecordHeader *recordHeader,
                      _pmdEDUCB *eduCB ) ;

      INT32 replayWriteLob( const CHAR *fullName,
                            const bson::OID &oid,
                            UINT32 sequence,
                            UINT32 offset,
                            UINT32 len,
                            const CHAR *data,
                            _pmdEDUCB *eduCB ) ;

   private:
      _SDB_DMSCB              *_dmsCB ;
      _dpsLogWrapper          *_dpsCB ;
      monDBCB                 *_monDBCB ;

      BOOLEAN                 _isReplSync ;

   } ;
   typedef class _clsReplayer clsReplayer ;
}

#endif

