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

   Source File Name = rtnAlterFuncs.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/05/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_ALTERFUNCS_HPP_
#define RTN_ALTERFUNCS_HPP_

#include "rtnAlterDef.hpp"

namespace engine
{
   class _pmdEDUCB ;
   class _dpsLogWrapper ;
   /// see func's name in msgDef.hpp

   /// SDB_ALTER_CRT_ID_INDEX
   INT32 rtnCreateIDIndex( const CHAR *name,
                           const bson::BSONObj &pubArgs,
                           const bson::BSONObj &args,
                           _pmdEDUCB *cb,
                           _dpsLogWrapper *dpsCB ) ;

   INT32 rtnCreateIDIndexVerify( const bson::BSONObj &args ) ;

   /// SDB_ALTER_DROP_ID_INDEX
   INT32 rtnDropIDIndex( const CHAR *name,
                         const bson::BSONObj &pubArgs,
                         const bson::BSONObj &args,
                         _pmdEDUCB *cb,
                         _dpsLogWrapper *dpsCB ) ;

   INT32 rtnDropIDIndexVerify( const bson::BSONObj &args )
   {
      return SDB_OK ;
   }
}

#endif

