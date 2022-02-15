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

   Source File Name = threadContext.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_THREAD_CONTEXT_H_
#define VESSEL_THREAD_CONTEXT_H_

#include "sdbInterface.hpp"
#include "vessel/simpleBufferAllocator.h"
#include "vessel/shallowPointer.hpp"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   class instanceEnv;
   class threadContextGuard;

   class threadContext : public SDBObject
   {
      friend class threadContextGuard;
         
      public:
         threadContext(IExecutor *executor,
                       instanceEnv *env);
         ~threadContext(){}
         threadContext(const threadContext &) = delete;
         threadContext &operator=(const threadContext &) = delete;

      public:
         OSS_INLINE IExecutor *getExecutor() {return _executor;}
         OSS_INLINE instanceEnv *getEnv() {return _env;}
         OSS_INLINE BOOLEAN isValid()const {return nullptr != _executor;}
         
      public:
         CHAR *allocateBuffer(UINT32 size);
         void releaseBuffer(void *buffer);

         template<class T>
         shallowArray<T> allocateArray(UINT32 elementCount)
         {
            shallowArray<T> arr;
            const CHAR *buffer = this->allocateBuffer(elementCount * sizeof(T));
            if (OSS_LIKELY(nullptr != buffer))
            {
               arr = shallowArray<T>((T*)buffer, elementCount);
            }
            return arr;
         }

      public:
         DPS_TRANS_ID getTransIDOfExecutor()const;

      private:
         static constexpr UINT32 _S_BUF_POOL_SIZE = 8192;

      private:
         IExecutor *_executor = nullptr;
         instanceEnv *_env = nullptr;
         CHAR _staticBuf[_S_BUF_POOL_SIZE];
         simpleBufferAllocator _sba;
         BOOLEAN _attached = FALSE;
   };//class threadContext
   typedef class threadContext THREAD_CONTEXT;

   

   class threadContextGuard : public SDBObject
   {
      public:
         threadContextGuard(IExecutor *executor,
                            instanceEnv *env);
         ~threadContextGuard();
         threadContextGuard(const threadContextGuard &) = delete;
         threadContextGuard &operator=(const threadContextGuard &) = delete;

      public:
         static THREAD_CONTEXT *getContext(){return _T_CONTEXT;}

      private:
         THREAD_CONTEXT _context;
         static OSS_THREAD_LOCAL THREAD_CONTEXT *_T_CONTEXT;
   };//class threadContextGuard
   OSS_THREAD_LOCAL threadContext *threadContextGuard::_T_CONTEXT = nullptr;

   OSS_INLINE THREAD_CONTEXT *GET_THREAD_CONTEXT()
   {
      return threadContextGuard::getContext();
   }
   
} // namespace vessel

} // namespace engine


#endif//VESSEL_THREAD_CONTEXT_H_