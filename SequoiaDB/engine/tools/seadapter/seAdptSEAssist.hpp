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

   Source File Name = seAdptSEAssist.hpp

   Descriptive Name = Search engine assistant for search engine adapter.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/14/2019  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SEADPT_SEASSIST_HPP__
#define SEADPT_SEASSIST_HPP__

#include "utilESBulkBuilder.hpp"
#include "utilESClt.hpp"
#include "seAdptOprMon.hpp"
#include "utilESCltMgr.hpp"

namespace seadapter
{
   /**
    * @brief Search engine assistant. It will maintain a connection with the
    * search engine, and send requests to it.
    */
   class _seAdptSEAssist : public SDBObject
   {
   public:
      _seAdptSEAssist() ;
      ~_seAdptSEAssist() ;

      INT32 init( UINT32 bulkBuffSz = SEADPT_DFT_BULKBUFF_SZ ) ;
      INT32 createIndex( const CHAR *name, const CHAR *mapping = NULL ) ;
      INT32 dropIndex( const CHAR *name ) ;
      INT32 indexExist( const CHAR *name, BOOLEAN &exist ) ;
      INT32 getDocument( const CHAR *index, const CHAR *type, const CHAR *id,
                         BSONObj &result ) ;

      INT32 indexDocument( const CHAR *index, const CHAR *type, const CHAR *id,
                           const CHAR *jsonData ) ;

      INT32 bulkPrepare( const CHAR *index, const CHAR *type ) ;
      INT32 bulkProcess( const utilESBulkActionBase &actionItem ) ;
      INT32 processBigItem( const utilESBulkActionBase &actionItem ) ;
      INT32 bulkAppendIndex( const CHAR *docID, const BSONObj &doc ) ;
      INT32 bulkAppendDel( const CHAR *docID ) ;
      INT32 bulkAppendReplace( const CHAR *id, const CHAR *newId,
                               const BSONObj &doc ) ;
      INT32 bulkFinish() ;

      seAdptOprMon* oprMonitor()
      {
         return &_oprMon ;
      }

   private:
      utilESCltMgr *_esCltMgr ;
      UINT32 _bulkBuffSz ;
      utilESBulkBuilder _bulkBuilder ;
      CHAR _index[ SEADPT_MAX_IDXNAME_SZ + 1 ] ;
      CHAR _type[ SEADPT_MAX_TYPE_SZ + 1 ] ;
      seAdptOprMon _oprMon ;
   } ;
   typedef _seAdptSEAssist seAdptSEAssist ;
}

#endif /* SEADPT_SEASSIST_HPP__ */

