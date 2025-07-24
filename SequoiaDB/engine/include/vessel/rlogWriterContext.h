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

   Source File Name = rlogWriterContext.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_RLOG_WRITER_CONTEXT_H_
#define VESSEL_RLOG_WRITER_CONTEXT_H_

#include "dpsDef.hpp"
#include <atomic>
#include <condition_variable>
#include <mutex>

namespace engine
{
namespace vessel
{
   class rlogWriterContext : public SDBObject
   {
      public:
         struct event
         {
            OSS_INLINE void reset()
            {
               writeOffset = 0;
               flushOffset = 0;
               return;
            }

            OSS_INLINE BOOLEAN isActive() const
            {
               return 0 < writeOffset || 0 < flushOffset;
            }

            UINT64 writeOffset = 0;
            UINT64 flushOffset = 0;
         };

      public:
         void reset();

         void request(UINT64 offset, BOOLEAN flushAtOnce);

         /// return false if quitintg
         BOOLEAN wait(UINT32 millis, BOOLEAN &timeout, event &e);

         void attach();

         void detach();

         void quitWriter();

         OSS_INLINE BOOLEAN isWriterAttached() const
         {
            return _attached.load();
         }
      private:
         std::atomic_bool _attached = {false};
         std::mutex _mutex;
         std::condition_variable _cv;
         event _event;
         BOOLEAN _quit = FALSE;
   };//class rlogWriterContext
} // namespace vessel

} // namespace engine


#endif//VESSEL_RLOG_WRITER_CONTEXT_H_