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

   Source File Name = clsOprHandler.hpp

   Descriptive Name = Operation Handler Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains declare for runtime
   functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/05/2022  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CLS_OPRHANDLER_HPP_
#define CLS_OPRHANDLER_HPP_

#include "rtnOprHandler.hpp"
#include "clsShardSession.hpp"

namespace engine
{
   class _ixmIndexKeyGen ;

   class _clsOprHandler : public IRtnOprHandler
   {
   public:
      _clsOprHandler( _clsShdSession *shdSession, const CHAR *clName,
                      BOOLEAN changeShardingKey ) ;
      _clsOprHandler( _clsShdSession *shdSession, const CHAR *mainCLName,
                      const CHAR *subCLName, BOOLEAN changeShardingKey ) ;

      virtual ~_clsOprHandler() {}

      INT32 getShardingKey( const CHAR *clName, BSONObj &shardingKey ) ;

      virtual void  onCSClosed( INT32 csID )  ;

      virtual void  onCLTruncated( INT32 csID, UINT16 clID )  ;

      virtual INT32 onCreateIndex( _dmsMBContext *context,
                                   const ixmIndexCB *indexCB,
                                   _pmdEDUCB *cb )  ;

      virtual INT32 onDropIndex( _dmsMBContext *context,
                                 const ixmIndexCB *indexCB,
                                 _pmdEDUCB *cb )  ;

      virtual INT32 onRebuildIndex( _dmsMBContext *context,
                                    const ixmIndexCB *indexCB,
                                    _pmdEDUCB *cb,
                                    utilWriteResult *pResult )  ;

      virtual INT32 onInsertRecord( _dmsMBContext *context,
                                    const BSONObj &object,
                                    const dmsRecordID &rid,
                                    const _dmsRecordRW *pRecordRW,
                                    _pmdEDUCB* cb )  ;

      virtual INT32 onDeleteRecord( _dmsMBContext *context,
                                    const BSONObj &object,
                                    const dmsRecordID &rid,
                                    const _dmsRecordRW *pRecordRW,
                                    BOOLEAN markDeleting,
                                    _pmdEDUCB* cb )  ;

      virtual INT32 onUpdateRecord( _dmsMBContext *context,
                                    const BSONObj &orignalObj,
                                    const BSONObj &newObj,
                                    const dmsRecordID &rid,
                                    const _dmsRecordRW *pRecordRW,
                                    _pmdEDUCB *cb ) ;

      virtual INT32 onInsertIndex( _dmsMBContext *context,
                                   const ixmIndexCB *indexCB,
                                   BOOLEAN isUnique,
                                   BOOLEAN isEnforce,
                                   const BSONObjSet &keySet,
                                   const dmsRecordID &rid,
                                   _pmdEDUCB* cb,
                                   utilWriteResult *pResult )  ;

      virtual INT32 onInsertIndex( _dmsMBContext *context,
                                   const ixmIndexCB *indexCB,
                                   BOOLEAN isUnique,
                                   BOOLEAN isEnforce,
                                   const BSONObj &keyObj,
                                   const dmsRecordID &rid,
                                   _pmdEDUCB* cb,
                                   utilWriteResult *pResult )  ;

      virtual INT32 onUpdateIndex( _dmsMBContext *context,
                                   const ixmIndexCB *indexCB,
                                   BOOLEAN isUnique,
                                   BOOLEAN isEnforce,
                                   const BSONObjSet &oldKeySet,
                                   const BSONObjSet &newKeySet,
                                   const dmsRecordID &rid,
                                   BOOLEAN isRollback,
                                   _pmdEDUCB* cb,
                                   utilWriteResult *pResult )  ;

      virtual INT32 onDeleteIndex( _dmsMBContext *context,
                                   const ixmIndexCB *indexCB,
                                   BOOLEAN isUnique,
                                   const BSONObjSet &keySet,
                                   const dmsRecordID &rid,
                                   _pmdEDUCB* cb )  ;
   private:
      INT32 _init() ;
      INT32 _prepareShardKeyKeygen( const CHAR *clName,
                                    _ixmIndexKeyGen &keygen ) ;

      INT32 _getAndValidShardKeyChange( _ixmIndexKeyGen &keygen,
                                        const BSONObj &originalObj,
                                        const BSONObj &newObj,
                                        _pmdEDUCB *cb ) ;

   private:
      CHAR                 _mainCLName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] ;
      CHAR                 _clName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] ;
      BOOLEAN              _checkShardingKey ;
      BOOLEAN              _isInit ;
      _ixmIndexKeyGen      _mainCLKeygen ;
      _ixmIndexKeyGen      _clKeygen ;
      _clsShdSession      *_shardSession ;
   } ;
   typedef _clsOprHandler clsOprHandler ;
}

#endif /* CLS_OPRHANDLER_HPP_ */
