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

   Source File Name = rtnLob.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/31/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_LOB_HPP_
#define RTN_LOB_HPP_

#include "rtn.hpp"
#include "dmsLobDef.hpp"

namespace engine
{
   /// interfaces for stream
   INT32 rtnOpenLob( const BSONObj &lob,
                     SINT32 flags,
                     BOOLEAN isLocal,
                     _pmdEDUCB *cb,
                     SDB_DPSCB *dpsCB,
                     SINT16 w,
                     SINT64 &contextID,
                     rtnContextBuf &buffObj ) ;

   INT32 rtnWriteLob( SINT64 contextID,
                      pmdEDUCB *cb,
                      UINT32 len,
                      const CHAR *buf ) ;

   INT32 rtnReadLob( SINT64 contextID,
                     pmdEDUCB *cb,
                     UINT32 len,
                     SINT64 offset,
                     const CHAR **buf,
                     UINT32 &read ) ;

   INT32 rtnCloseLob( SINT64 contextID,
                      pmdEDUCB *cb ) ;

   INT32 rtnRemoveLob( const BSONObj &lob,
                       INT32 flags,
                       SINT16 w,
                       _pmdEDUCB *cb,
                       SDB_DPSCB *dpsCB ) ;

   INT32 rtnGetLobMetaData( SINT64 contextID,
                            pmdEDUCB *cb,
                            BSONObj &meta ) ;

   /// interfaces for non-stream
   INT32 rtnCreateLob( const CHAR *fullName,
                       const bson::OID &oid,
                       pmdEDUCB *cb,
                       SINT16 w,
                       SDB_DPSCB *dpsCB,
                       dmsStorageUnit *su = NULL,
                       dmsMBContext *mbContext = NULL ) ;

   INT32 rtnGetLobMetaData( const CHAR *fullName,
                            const bson::OID &oid,
                            pmdEDUCB *cb,
                            dmsLobMeta &meta,
                            dmsStorageUnit *su = NULL,
                            dmsMBContext *mbContext = NULL ) ;

   INT32 rtnWriteLob( const CHAR *fullName,
                      const bson::OID &oid,
                      UINT32 sequence,
                      UINT32 offset,
                      UINT32 len,
                      const CHAR *data,
                      pmdEDUCB *cb,
                      SINT16 w,
                      SDB_DPSCB *dpsCB,
                      dmsStorageUnit *su = NULL,
                      dmsMBContext *mbContext = NULL ) ;

   INT32 rtnUpdateLob( const CHAR *fullName,
                       const bson::OID &oid,
                       UINT32 sequence,
                       UINT32 offset,
                       UINT32 len,
                       const CHAR *data,
                       pmdEDUCB *cb,
                       SINT16 w,
                       SDB_DPSCB *dpsCB,
                       dmsStorageUnit *su = NULL,
                       dmsMBContext *mbContext = NULL ) ;

   INT32 rtnReadLob( const CHAR *fullName,
                     const bson::OID &oid,
                     UINT32 sequence,
                     UINT32 offset,
                     UINT32 len,
                     pmdEDUCB *cb,
                     CHAR *data,
                     UINT32 &read,
                     dmsStorageUnit *su = NULL,
                     dmsMBContext *mbContext = NULL ) ;
                      

   INT32 rtnCloseLob( const CHAR *fullName,
                      const bson::OID &oid,
                      const dmsLobMeta &meta,
                      pmdEDUCB *cb,
                      SINT16 w,
                      SDB_DPSCB *dpsCB,
                      dmsStorageUnit *su = NULL,
                      dmsMBContext *mbContext = NULL ) ;

   INT32 rtnRemoveLobPiece( const CHAR *fullName,
                            const bson::OID &oid,
                            UINT32 sequence,
                            pmdEDUCB *cb,
                            SINT16 w,
                            SDB_DPSCB *dpsCB,
                            dmsStorageUnit *su = NULL,
                            dmsMBContext *mbContext = NULL ) ;

   INT32 rtnQueryAndInvalidateLob( const CHAR *fullName,
                                   const bson::OID &oid,
                                   pmdEDUCB *cb,
                                   SINT16 w,
                                   SDB_DPSCB *dpsCB,
                                   dmsLobMeta &meta,
                                   dmsStorageUnit *su = NULL,
                                   dmsMBContext *mbContext = NULL ) ;


}

#endif

