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

   Source File Name = fapMongoUtil.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== ========================================
          11/04/2021  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef _SDB_MSG_BUFFER_HPP_
#define _SDB_MSG_BUFFER_HPP_

#include "oss.hpp"
#include "ossUtil.hpp"
#include "ossMem.hpp"
#include "../../bson/bson.hpp"
#include "utilCommon.hpp"
#include "rtnContextBuff.hpp"
#include "fapMongoCommandDef.hpp"
#include "mthMatchTree.hpp"

#define MEMERY_BLOCK_SIZE 4096

namespace fap
{

/*
   _mongoMsgBuffer define
*/
class _mongoMsgBuffer : public engine::_utilPooledObject
{
public:
   _mongoMsgBuffer() ;
   ~_mongoMsgBuffer() ;

   INT32 write( const CHAR *pIn, const UINT32 inLen,
                BOOLEAN align = FALSE, INT32 bytes = 4 ) ;

   INT32 write( const BSONObj &obj,
                BOOLEAN align = FALSE, INT32 bytes = 4 ) ;

   INT32 advance( const UINT32 pos ) ;

   void zero() ;

   INT32 reserve( const UINT32 size ) ;

   BOOLEAN empty() const
   {
      return 0 == _size ;
   }

   CHAR *data() const
   {
      return _pData ;
   }

   const UINT32 size() const
   {
      return _size ;
   }

   const UINT32 capacity() const
   {
      return _capacity ;
   }

   void doneLen()
   {
      *(SINT32 *)_pData = _size ;
   }

private:
   INT32 _alloc( const UINT32 size ) ;
   INT32 _realloc( const UINT32 size ) ;

private:
   CHAR  *_pData ;
   UINT32 _size ;
   UINT32 _capacity ;
} ;
typedef _mongoMsgBuffer mongoMsgBuffer ;

void mongoInitMsgHeader( MsgHeader *pMsg, INT32 opCode, UINT64 reqID = 0, UINT32 tid = 0 ) ;

class _mongoErrorObjAssit : public SDBObject
{
public:
   _mongoErrorObjAssit() ;
   ~_mongoErrorObjAssit(){ release() ; }

   void release() ;
   BSONObj getErrorObj( INT32 errorCode ) ;

private:
   BSONObj _errorObjsArray[ SDB_MAX_ERROR + SDB_MAX_WARNING + 1 ] ;
};
typedef _mongoErrorObjAssit mongoErrorObjAssit ;

void  mongoReleaseErrorBson() ;

struct fapFieldMapItem
{
   fapFieldMapItem( const CHAR* const m, const CHAR* const s, const BOOLEAN c )
   : mongoField( m ), sdbField( s ), canPushDown( c ) {}

   // { NULL, NULL, FALSE } is the end of array
   const CHAR* const mongoField ;
   const CHAR* const sdbField ;
   const BOOLEAN     canPushDown ;
} ;

/*
   _mongoFilterHelper define
*/
class _mongoFilterHelper : public SDBObject
{
public:
   _mongoFilterHelper() ;

   ~_mongoFilterHelper(){}

   INT32 loadPattern( const BSONObj &pattern ) ;

   INT32 matches( const BSONObj &matchTarget, BOOLEAN &result ) ;

private:
   engine::mthMatchTree _matchTree ;
} ;
typedef _mongoFilterHelper mongoFilterHelper ;

INT32 mongoGenerateNewRecord( const BSONObj &matcher,
                              const BSONObj &updatorObj,
                              const BSONObj &setOnInsert,
                              BSONObj &target ) ;

BSONObj mongoGetErrorBson( INT32 errorCode, const CHAR *pErrMsg = NULL ) ;

void    mongoBuildErrorBson( BSONObjBuilder &builder, INT32 errorCode,
                             const CHAR *pErrMsg = NULL,
                             const BSONObj &objDetail = BSONObj() ) ;

BOOLEAN mongoCheckBigEndian() ;

INT32 mongoGetIntElement( const BSONObj &obj, const CHAR *pFieldName,
                          INT32 &value ) ;

INT32 mongoGetStringElement ( const BSONObj &obj, const CHAR *pFieldName,
                              const CHAR *&pValue ) ;

INT32 mongoGetArrayElement ( const BSONObj &obj, const CHAR *pFieldName,
                             BSONObj &value ) ;

INT32 mongoGetNumberLongElement ( const BSONObj &obj,
                                  const CHAR *pFieldName,
                                  INT64 &value ) ;

INT32 mongoGetBooleanElement ( const BSONObj &obj, const CHAR *fieldName,
                               BOOLEAN &value ) ;

INT32 mongoGetObjElement ( const BSONObj &obj, const CHAR *fieldName,
                           BSONObj &value ) ;

INT32 mongoBuildDupkeyErrObj( const BSONObj &sdbErrobj, const CHAR* clFullName,
                              BSONObjBuilder &builder ) ;

INT32 mongoCheckUpdator( BSONObj &updator, BOOLEAN &hasOp, BSONObj &setOnInsert ) ;

/*
   Caller show try/catch
*/
void  mongoFixInsertObject( const BSONObj &inObj, BSONObjBuilder &builder,
                            BOOLEAN& hasRebuildOID, BSONObj *pOutObj = NULL ) ;

INT32 mongoRebuildOKReply( engine::rtnContextBuf &bodyBuf ) ;

std::string mongoGetNonce() ;

INT32 utilSdbRC2MongoRC( INT32 sdbRC ) ;

}
#endif
