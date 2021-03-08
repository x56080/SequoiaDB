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

   Source File Name = requestHandler.h

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

#ifndef VESSEL_REQUEST_HANDLER_H_
#define VESSEL_REQUEST_HANDLER_H_

#include "vessel/vesselDef.h"
#include "vessel/requestContext.h"

namespace engine
{
namespace vessel
{
   class instanceEnv;
   class ISession;

   class requestHandler : public SDBObject
   {
      public:
         OSS_INLINE requestHandler()
         {}

         virtual ~requestHandler()
         {
            fini();
         }

      public:
         INT32 init(instanceEnv *env,
                     ISession *session,
                     outerResource *outer);

         virtual INT32 fini();

         OSS_INLINE BOOLEAN isInitialized()const
         {
            return _context.isOpen();
         }
      protected:
         OSS_INLINE instanceEnv *getEnv()
         {
            return isInitialized() ? _context.getEnv() : NULL;
         }

         OSS_INLINE ISession *getSession()
         {
            return isInitialized() ? _context.getSession() : NULL;
         }

         OSS_INLINE requestContext *getContext()
         {
            return &_context;
         }

      private:
         requestContext _context;
   };//class requestHandler
}//namespace vessel
}//namespace engine

#endif //VESSEL_REQUEST_HANDLER_H_