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

   Source File Name = clsRecycleBinManager.hpp

   Descriptive Name = Recycle Bin Manager Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/01/2021  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLS_RECYCLE_BIN_MGR_HPP__
#define CLS_RECYCLE_BIN_MGR_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "pmdEDU.hpp"
#include "rtnRecycleBinManager.hpp"
#include "dmsLocalSUMgr.hpp"

namespace engine
{

   class _clsMgr ;

   /*
      _clsRecycleBinManager define
    */
   class _clsRecycleBinManager : public _rtnRecycleBinManager
   {
   protected:
         typedef _rtnRecycleBinManager _BASE ;

   public:
      _clsRecycleBinManager() ;
      virtual ~_clsRecycleBinManager() ;

   protected:
      virtual const CHAR *_getRecyItemCL() const
      {
         return DMS_SYSLOCALRECYCLEITEM_CL_NAME ;
      }
   } ;

   typedef class _clsRecycleBinManager clsRecycleBinManager ;

}

#endif // CLS_RECYCLE_BIN_MGR_HPP__
