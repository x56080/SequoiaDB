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

#include "core.hpp"
#include "oss.hpp"
#include "sdbInterface.hpp"
#include "vessel/shallowPointer.hpp"
#include "vessel/requestContext.h"
#include "vessel/collection.h"


namespace engine
{
namespace vessel
{
   class instanceEnv;
   class outerResource;

   class requestHandler : public SDBObject
   {
      public:
         OSS_INLINE requestHandler()
         {}
         virtual ~requestHandler()
         {
         }

         requestHandler(const requestHandler &o) = delete;
         requestHandler &operator=(const requestHandler &o) = delete;

      public:
         void init(instanceEnv *env,
                   IExecutor *executor,
                   outerResource *outer);

         BOOLEAN isInitialized()const;

      protected:
         OSS_INLINE instanceEnv *getEnv()
         {
            return _env;
         }
         OSS_INLINE IExecutor *getExecutor()
         {
            return _executor;
         }
         OSS_INLINE outerResource *getOuterResource()
         {
            return _outerResource;
         }

      protected:
         
         INT32 getCollectionObject(requestContext *context,
                                   const globalCollectionId &gcid,
                                   OSS_LATCH_MODE mode,
                                   COLLECTION_PTR &out);
      private:
         IExecutor *_executor = NULL;
         instanceEnv *_env = NULL;
         outerResource *_outerResource = NULL;
         
   };//class requestHandler
}//namespace vessel
}//namespace engine

#endif //VESSEL_REQUEST_HANDLER_H_