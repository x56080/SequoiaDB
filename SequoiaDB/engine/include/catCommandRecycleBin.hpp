/*******************************************************************************


   Copyright (C) 2011-2021 SequoiaDB Ltd.

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

   Source File Name = catCommandRecycleBin.hpp

   Descriptive Name = Catalogue commands for recycle bin

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime code for catalog
   commands.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/01/2021  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CAT_COMMAND_RECYCLEBIN_HPP__
#define CAT_COMMAND_RECYCLEBIN_HPP__

#include "catCMDBase.hpp"
#include "catRecycleBinManager.hpp"
#include "utilRecycleBinConf.hpp"

namespace engine
{

   /*
      _catCMDGetRecycleBinDetail define
    */
   class _catCMDGetRecycleBinDetail : public _catReadCMDBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER() ;

   public:
      _catCMDGetRecycleBinDetail() ;
      virtual ~_catCMDGetRecycleBinDetail() ;

      INT32 init( const CHAR *pQuery,
                  const CHAR *pSelector = NULL,
                  const CHAR *pOrderBy = NULL,
                  const CHAR *pHint = NULL,
                  INT32 flags = 0,
                  INT64 numToSkip = 0,
                  INT64 numToReturn = -1 )
      {
         return SDB_OK ;
      }

      INT32 doit( _pmdEDUCB *cb,
                  rtnContextBuf &ctxBuf,
                  INT64 &contextID ) ;

      virtual const CHAR *name() const
      {
         return CMD_NAME_GET_RECYCLEBIN_DETAIL ;
      }

   protected:
      catRecycleBinManager * _recycleBinMgr ;
   } ;

   typedef class _catCMDGetRecycleBinDetail catCMDGetRecycleBinDetail ;

   /*
      _catCMDAlterRecycleBin define
    */
   class _catCMDAlterRecycleBin : public _catWriteCMDBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER() ;

   public:
      _catCMDAlterRecycleBin() ;
      virtual ~_catCMDAlterRecycleBin() ;

      INT32 init( const CHAR *pQuery,
                  const CHAR *pSelector = NULL,
                  const CHAR *pOrderBy = NULL,
                  const CHAR *pHint = NULL,
                  INT32 flags = 0,
                  INT64 numToSkip = 0,
                  INT64 numToReturn = -1 ) ;

      INT32 doit( _pmdEDUCB *cb,
                  rtnContextBuf &ctxBuf,
                  INT64 &contextID ) ;

      virtual const CHAR *name() const
      {
         return CMD_NAME_ALTER_RECYCLEBIN ;
      }

   protected:
      catRecycleBinManager *  _recycleBinMgr ;
      // new recycle bin conf after alter
      utilRecycleBinConf      _newConf ;
   } ;

   typedef class _catCMDAlterRecycleBin catCMDAlterRecycleBin ;

   /*
      _catCMDGetRecycleBinCount define
    */
   class _catCMDGetRecycleBinCount : public _catReadCMDBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER() ;

   public:
      _catCMDGetRecycleBinCount() ;
      virtual ~_catCMDGetRecycleBinCount() ;

      INT32 init( const CHAR *pQuery,
                  const CHAR *pSelector = NULL,
                  const CHAR *pOrderBy = NULL,
                  const CHAR *pHint = NULL,
                  INT32 flags = 0,
                  INT64 numToSkip = 0,
                  INT64 numToReturn = -1 ) ;

      INT32 doit( _pmdEDUCB *cb,
                  rtnContextBuf &ctxBuf,
                  INT64 &contextID ) ;

      virtual const CHAR *name() const
      {
         return CMD_NAME_GET_RECYCLEBIN_COUNT ;
      }

   protected:
      // cache of command options
      bson::BSONObj _queryObj ;
   } ;

   typedef class _catCMDGetRecycleBinCount catCMDGetRecycleBinCount ;

}

#endif // CAT_COMMAND_RECYCLEBIN_HPP__
