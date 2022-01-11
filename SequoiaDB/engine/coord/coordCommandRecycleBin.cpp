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

}
