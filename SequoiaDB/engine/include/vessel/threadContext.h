/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = threadContext.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
   class threadContextOnwer;

   class threadContext : public SDBObject
   {
      friend class threadContextOnwer;
         
      public:
         threadContext();
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

         BOOLEAN hasUnfreeBuffer()const;

      public:
         DPS_TRANS_ID getTransID()const;
         
      private:
         static constexpr UINT32 _S_BUF_POOL_SIZE = 8192;

      private:
         IExecutor *_executor = nullptr;
         instanceEnv *_env = nullptr;
         CHAR _staticBuf[_S_BUF_POOL_SIZE];
         simpleBufferAllocator _sba;
   };//class threadContext
   typedef class threadContext THREAD_CONTEXT;

   

   class threadContextOnwer : public SDBObject
   {
      public:
         threadContextOnwer(IExecutor *executor,
                            instanceEnv *env);
         ~threadContextOnwer();
         threadContextOnwer(const threadContextOnwer &) = delete;
         threadContextOnwer &operator=(const threadContextOnwer &) = delete;

      public:
         static THREAD_CONTEXT *getContext(){return _T_CONTEXT;}

      private:
         THREAD_CONTEXT _context;
         static OSS_THREAD_LOCAL THREAD_CONTEXT *_T_CONTEXT;
   };//class threadContextOnwer

   typedef class threadContextOnwer THREAD_CONTEXT_OWNER;

   OSS_INLINE THREAD_CONTEXT *GET_THREAD_CONTEXT()
   {
      return threadContextOnwer::getContext();
   }
   
} // namespace vessel

} // namespace engine


#endif//VESSEL_THREAD_CONTEXT_H_