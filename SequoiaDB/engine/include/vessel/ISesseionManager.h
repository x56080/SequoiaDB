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

   Source File Name = ISessionManager.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_I_SESSION_MANAGER_H_
#define VESSEL_I_SESSION_MANAGER_H_

#include "vessel/ISession.h"
#include <thread>

namespace engine
{
namespace vessel
{
   class ISessionManager : public SDBObject
   {
      public:
         ISessionManager(){}
         virtual ~ISessionManager(){}

      public:
         virtual ISession *createNewSession() = 0;
         virtual void destroySession(ISession *session) = 0;
   };//class ISessionManager
}//namespace vessel
}//namespace engine

#endif//VESSEL_I_SESSION_MANAGER_H_