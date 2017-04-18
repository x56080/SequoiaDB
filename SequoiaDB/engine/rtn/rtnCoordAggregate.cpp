/*******************************************************************************

   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = rtnCoordAggregate.cpp

   Descriptive Name = Runtime Coord Aggregation

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   aggregation logic on coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#include "rtnCoordAggregate.hpp"
#include "msgMessage.hpp"
#include "pmd.hpp"
#include "rtnCommandDef.hpp"
#include "pmdCB.hpp"

using namespace bson;
namespace engine
{
   INT32 rtnCoordAggregate::execute( MsgHeader *pMsg,
                                     pmdEDUCB *cb,
                                     INT64 &contextID,
                                     rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK;
      CHAR *pCollectionName = NULL;
      CHAR *pObjs = NULL;
      INT32 count = 0;
      BSONObj objs;
      INT32 flags = 0 ;

      contextID = -1 ;

      rc = msgExtractAggrRequest( (CHAR*)pMsg, &pCollectionName,
                                  &pObjs, count, &flags ) ;
      PD_RC_CHECK( rc, PDERROR, "failed to parse aggregate request(rc=%d)", rc );

      try
      {
         objs = BSONObj( pObjs ) ;

         /// Prepare last info
         CHAR szTmp[ MON_APP_LASTOP_DESC_LEN + 1 ] = { 0 } ;
         UINT32 len = 0 ;
         const CHAR *pObjData = pObjs ;
         for ( INT32 i = 0 ; i < count ; ++i )
         {
            BSONObj tmpObj( pObjData ) ;
            len += ossSnprintf( szTmp, MON_APP_LASTOP_DESC_LEN - len,
                                "%s", tmpObj.toString().c_str() ) ;
            pObjData += ossAlignX( (UINT32)tmpObj.objsize(), 4 ) ;
            if ( len >= MON_APP_LASTOP_DESC_LEN )
            {
               break ;
            }
         }
         // add last op info
         MON_SAVE_OP_DETAIL( cb->getMonAppCB(), pMsg->opCode,
                             "Collection:%s, ObjNum:%u, Objs:%s, "
                             "Flag:0x%08x(%u)",
                             pCollectionName, count, szTmp,
                             flags, flags ) ;

         rc = pmdGetKRCB()->getAggrCB()->build( objs, count, pCollectionName,
                                                cb, contextID ) ;
         /// AUDIT
         PD_AUDIT_OP( AUDIT_DQL, pMsg->opCode, AUDIT_OBJ_CL,
                      pCollectionName, rc,
                      "ContextID:%lld, ObjNum:%u, Objs:%s, Flag:0x%08x(%u)",
                      contextID, count, szTmp, flags, flags ) ;
         /// CHECK RESULT
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to execute aggregation operation(rc=%d)",
                      rc );
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK( SDB_SYS, PDERROR,
                      "Failed to execute aggregate, received unexpecte error:%s",
                      e.what() );
      }

   done:
      return rc;
   error:
      if ( contextID >= 0 )
      {
         pmdGetKRCB()->getRTNCB()->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      goto done;
   }
}
