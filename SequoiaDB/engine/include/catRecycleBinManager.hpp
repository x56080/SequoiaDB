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

   Source File Name = catRecycleBinManager.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for catalog node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/03/2021  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CAT_RECYCLE_BIN_MANAGER_HPP__
#define CAT_RECYCLE_BIN_MANAGER_HPP__

#include "oss.hpp"
#include "catDef.hpp"
#include "utilRecycleBinConf.hpp"
#include "rtnRecycleBinManager.hpp"
#include "utilRecycleItem.hpp"
#include "catLevelLock.hpp"
#include "pmdEDU.hpp"

namespace engine
{

   // pre-define
   class sdbCatalogueCB ;

   /*
      _catRecycleBinManager define
    */
   class _catRecycleBinManager : public _rtnRecycleBinManager
   {
   protected:
      typedef _rtnRecycleBinManager _BASE ;

   public:
      _catRecycleBinManager() ;
      virtual ~_catRecycleBinManager() ;

      INT32 init( const utilRecycleBinConf &conf ) ;

      // update config of recycle bin
      INT32 updateConf( const utilRecycleBinConf &newConf,
                        pmdEDUCB *cb,
                        INT16 w ) ;

   protected:
      virtual const CHAR *_getRecyItemCL() const
      {
         return CAT_SYSRECYCLEBIN_ITEM_COLLECTION ;
      }

   } ;

   typedef class _catRecycleBinManager catRecycleBinManager ;

}

#endif // CAT_RECYCLE_BIN_MANAGER_HPP__
