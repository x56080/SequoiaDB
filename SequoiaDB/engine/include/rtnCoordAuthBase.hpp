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

   Source File Name = rtnCoordAuthBase.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/12/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTNCOORDAUTHBASE_HPP_
#define RTNCOORDAUTHBASE_HPP_

#include "rtnCoordOperator.hpp"

namespace engine
{
   class rtnCoordAuthBase : public rtnCoordOperator
   {
   protected:
      INT32 forward( MsgHeader *pMsg,
                     pmdEDUCB *cb,
                     INT32 msgType,
                     BOOLEAN sWhenNoPrimary,
                     INT64 &contextID ) ;

   } ;
}

#endif

