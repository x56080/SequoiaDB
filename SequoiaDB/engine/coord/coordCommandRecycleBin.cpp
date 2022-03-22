/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = coordCommandRecycleBin.cpp

   Descriptive Name = Coord Common

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/01/2021  HGM Init
   Last Changed =

*******************************************************************************/

#include "coordCommandRecycleBin.hpp"
#include "coordCB.hpp"
#include "coordUtil.hpp"
#include "msgMessage.hpp"
#include "catDef.hpp"
#include "utilRecycleBinConf.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"

namespace engine
{

   /*
      _coordGetRecycleBinDetail implement
    */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordGetRecycleBinDetail,
                                      CMD_NAME_GET_RECYCLEBIN_DETAIL,
                                      TRUE ) ;
   _coordGetRecycleBinDetail::_coordGetRecycleBinDetail()
   {
   }

   _coordGetRecycleBinDetail::~_coordGetRecycleBinDetail()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_COORDGETRECYCLEBINDETAIL__PREPROCESS, "_coordGetRecycleBinDetail::_preProcess" )
   INT32 _coordGetRecycleBinDetail::_preProcess( rtnQueryOptions &queryOpt,
                                                 string &clName,
                                                 BSONObj &outSelector )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_COORDGETRECYCLEBINDETAIL__PREPROCESS ) ;

      // do nothing

      PD_TRACE_EXITRC( SDB_COORDGETRECYCLEBINDETAIL__PREPROCESS, rc ) ;

      return rc ;
   }

   /*
      _coordAlterRecycleBin implement
    */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordAlterRecycleBin,
                                      CMD_NAME_ALTER_RECYCLEBIN,
                                      TRUE ) ;
   _coordAlterRecycleBin::_coordAlterRecycleBin()
   {
   }

   _coordAlterRecycleBin::~_coordAlterRecycleBin()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_COORDALTERRECYCLEBIN_EXECUTE, "_coordAlterRecycleBin::execute" )
   INT32 _coordAlterRecycleBin::execute( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         INT64 &contextID,
                                         rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_COORDALTERRECYCLEBIN_EXECUTE ) ;

      CoordGroupList groupList ;

      rc = executeOnCataGroup( pMsg, cb, NULL, NULL, TRUE, NULL, buf ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to execute command [%s] on catalog, "
                   "rc: %d", getName(), rc ) ;

      // update all groups
      rc = _pResource->updateGroupList( groupList, cb, NULL, TRUE, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update all data group, "
                   "rc: %d", rc ) ;

      rc = executeOnDataGroup( pMsg, cb, groupList, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to execute %s on data nodes, rc: %d",
                   getName(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_COORDALTERRECYCLEBIN_EXECUTE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _coordGetRecycleBinCount implement
    */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordGetRecycleBinCount,
                                      CMD_NAME_GET_RECYCLEBIN_COUNT,
                                      TRUE ) ;
   _coordGetRecycleBinCount::_coordGetRecycleBinCount()
   {
   }

   _coordGetRecycleBinCount::~_coordGetRecycleBinCount()
   {
   }

   /*
      _coordDropRecycleBinBase implement
    */
   _coordDropRecycleBinBase::_coordDropRecycleBinBase( BOOLEAN isDropAll )
   : _isDropAll( isDropAll ),
     _recycleItemName( NULL )
   {
   }

   _coordDropRecycleBinBase::~_coordDropRecycleBinBase()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_COORDDROPRECYBINBASE_EXECUTE, "_coordDropRecycleBinBase::execute" )
   INT32 _coordDropRecycleBinBase::execute( MsgHeader *pMsg,
                                            pmdEDUCB *cb,
                                            INT64 &contextID,
                                            rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_COORDDROPRECYBINBASE_EXECUTE ) ;

      CoordGroupList groupList, sucGroupList ;
      BOOLEAN needAllGroups = _isDropAll ;

      rc = _parseMessage( pMsg ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse message for command [%s], "
                   "rc: %d", getName(), rc ) ;

      rc = executeOnCataGroup( pMsg, cb, &groupList, NULL, TRUE, NULL, buf ) ;
      if ( SDB_RECYCLE_ITEMNOTEXISTS == rc )
      {
         // recycle bin item does not exist in CATALOG, send to all groups
         // to drop remaining items in DATA nodes
         rc = SDB_OK ;
         needAllGroups = TRUE ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to execute command [%s] on catalog, "
                   "rc: %d", getName(), rc ) ;

      if ( needAllGroups )
      {
         // use all groups
         rc = _pResource->updateGroupList( groupList, cb, NULL, TRUE, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to update all data group for "
                      "command [%s], rc: %d", getName(), rc ) ;
      }

      rc = executeOnDataGroup( pMsg, cb, groupList, TRUE, NULL, &sucGroupList,
                               NULL, buf ) ;
      if ( SDB_RECYCLE_ITEMNOTEXISTS == rc && sucGroupList.size() > 0 )
      {
         // at least one group found the item, we can ignore the error
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to execute command [%s] on data, "
                   "rc: %d", getName(), rc ) ;

   done:
      _doAudit( rc ) ;

      PD_TRACE_EXITRC( SDB_COORDDROPRECYBINBASE_EXECUTE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB_COORDDROPRECYBINBASE__PARSEMSG "_coordDropRecycleBinBase::_parseMessage" )
   INT32 _coordDropRecycleBinBase::_parseMessage( MsgHeader *pMsg )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_COORDDROPRECYBINBASE__PARSEMSG ) ;

      try
      {
         const CHAR *pQuery = NULL ;

         rc = msgExtractQuery( (const CHAR *)pMsg, NULL, NULL, NULL, NULL,
                               &pQuery, NULL, NULL, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse message for command [%s], "
                      "rc: %d", getName(), rc ) ;

         _options.init( pQuery ) ;

         if ( _isDropAll )
         {
            BSONElement element = _options.getField( FIELD_NAME_RECYCLE_NAME ) ;
            PD_CHECK( EOO == element.type(), SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse message, should not get field [%s] "
                      "from options", FIELD_NAME_RECYCLE_NAME ) ;
         }
         else
         {
            utilRecycleItem dummyItem ;

            BSONElement element = _options.getField( FIELD_NAME_RECYCLE_NAME ) ;
            PD_CHECK( String == element.type(), SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse message, failed to get field [%s]",
                      FIELD_NAME_RECYCLE_NAME ) ;
            _recycleItemName = element.valuestr() ;

            rc = dummyItem.fromRecycleName( _recycleItemName ) ;
            PD_LOG_MSG_CHECK( SDB_OK == rc, rc, error, PDERROR,
                              "Failed to parse recycle item name [%s], "
                              "rc: %d", _recycleItemName, rc ) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse message, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_COORDDROPRECYBINBASE__PARSEMSG, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB_COORDDROPRECYBINBASE__DOAUDIT "_coordDropRecycleBinBase::_doAudit" )
   INT32 _coordDropRecycleBinBase::_doAudit( INT32 rc )
   {
      PD_TRACE_ENTRY( SDB_COORDDROPRECYBINBASE__DOAUDIT ) ;

      PD_AUDIT_COMMAND( AUDIT_DDL, getName(), AUDIT_OBJ_RECYCLEBIN,
                        ( NULL != _recycleItemName ? _recycleItemName : "" ),
                        rc, "Option: %s", _options.toPoolString().c_str() ) ;

      PD_TRACE_EXIT( SDB_COORDDROPRECYBINBASE__DOAUDIT ) ;

      return SDB_OK ;
   }

   /*
      _coordDropRecycleBinItem implement
    */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordDropRecycleBinItem,
                                      CMD_NAME_DROP_RECYCLEBIN_ITEM,
                                      TRUE ) ;

   _coordDropRecycleBinItem::_coordDropRecycleBinItem()
   : _coordDropRecycleBinBase( FALSE )
   {
   }

   _coordDropRecycleBinItem::~_coordDropRecycleBinItem()
   {
   }

   /*
      _coordDropRecycleBin implement
    */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordDropRecycleBinAll,
                                      CMD_NAME_DROP_RECYCLEBIN_ALL,
                                      TRUE ) ;

   _coordDropRecycleBinAll::_coordDropRecycleBinAll()
   : _coordDropRecycleBinBase( TRUE )
   {
   }

   _coordDropRecycleBinAll::~_coordDropRecycleBinAll()
   {
   }

}
