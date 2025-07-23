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

   Source File Name = rtnExtOprData.hpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_EXTOPR_DATA__
#define RTN_EXTOPR_DATA__

#include "ossMemPool.hpp"
#include "utilPooledObject.hpp"
#include "../bson/bson.hpp"

using bson::BSONObj ;

namespace engine
{
   typedef void *    RTN_EXTOPR_HANDLE ;

   class _rtnExtOprData : public utilPooledObject
   {
      typedef ossPoolMap<void *, BSONObj>       OPR_RECORD_MAP ;
      typedef OPR_RECORD_MAP::const_iterator    OPR_RECORD_MAP_CITR ;
   public:
      _rtnExtOprData() ;
      virtual ~_rtnExtOprData() ;

   public:
      INT32 setOrigRecord( const BSONObj &record, BOOLEAN getOwned = FALSE ) ;
      INT32 setNewRecord( const BSONObj &record, BOOLEAN getOwned = FALSE ) ;

      const BSONObj& getOrigRecord() const ;
      const BSONObj& getNewRecord() const ;

      INT32 saveOprRecord( void *key, const BSONObj &value,
                         BOOLEAN getOwned = FALSE ) ;
      BSONObj getOprRecord( void *key ) const ;

   private:
      INT32 _setRecord( BSONObj &target, const BSONObj &source,
                        BOOLEAN getOwned ) ;

   private:
      BSONObj _origRecord ;
      BSONObj _newRecord ;
      OPR_RECORD_MAP _oprRecordMap ;
   } ;
   typedef _rtnExtOprData rtnExtOprData ;
}

#endif /* RTN_EXTOPR_DATA__ */