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

   Source File Name = impParser.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/11/2020  HJW Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_PACKER_HPP_
#define IMP_PACKER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "impCommon.hpp"
#include "impOptions.hpp"
#include "impRecordSharding.hpp"
#include "impLogFile.hpp"

namespace import
{
   typedef map<UINT32, BsonPageHeader*> PageMap ;

   class Packer : public SDBObject
   {
   public:
      Packer() ;

      ~Packer() ;

      INT32 init( Options* options, BsonPageQueue* freeQueue,
                  PageQueue* importQueue ) ;

      INT32 packing( bson* record ) ;

      BOOLEAN clearQueue( BOOLEAN allClear = TRUE ) ;

      inline void stop()
      {
         _stopped = TRUE ;
      }

      inline INT64 shardingNum()
      {
         return _shardingNum.fetch() ;
      }

      inline INT64 failedNum()
      {
         return _failedNum.fetch() ;
      }

      inline const string& logFileName() const
      {
         return _logFile.fileName() ;
      }

   private:
      INT32 _getHeaderId( bson* record, UINT32& headerId ) ;

      INT32 _initShardingMap() ;

      INT32 _initPageMap() ;

      void _pushToImportQueue( BsonPageHeader* pageHeader ) ;

   private:
      INT32             _mapNum ;
      BOOLEAN           _inited ;
      BOOLEAN           _stopped ;
      BOOLEAN           _needSharding ;

      Options*          _options ;
      BsonPageQueue*    _freeQueue ;
      PageQueue*        _importQueue ;

      ossAtomicSigned64 _shardingNum ;
      ossAtomicSigned64 _failedNum ;

      PageMap           _pageMap ;
      RecordSharding    _sharding ;
      LogFile           _logFile ;
   } ;
}

#endif /* IMP_PACKER_HPP_ */