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

   Source File Name = ISession.h

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_I_SESSION_H_
#define VESSEL_I_SESSION_H_

#include "core.hpp"
#include "oss.hpp"

namespace engine
{
namespace vessel
{
   class ISession : public SDBObject
   {
      public:
         ISession(){}

         virtual ~ISession(){}

      public:
         virtual BOOLEAN nowait()const = 0;
         virtual UINT64 getSessionID()const = 0;
         virtual BOOLEAN quit()const = 0;
         virtual void clearLastError() = 0;
         virtual void setLastError(INT32 rc, const CHAR *fmt, ...) = 0;
         virtual UINT64 getLastLSN()const = 0;
         virtual void waitForCurrentWritingId() = 0;

   };//class ISession
}//namespace vessel
}//namespace engine

#endif//VESSEL_I_SESSION_H_