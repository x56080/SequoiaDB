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

   Source File Name = rlogWriterContext.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/rlogWriterContext.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   void rlogWriterContext::reset()
   {
      _attached.store(false);
      _event.reset();
      _quit = FALSE;
      return;
   }

   void rlogWriterContext::request(UINT64 offset, BOOLEAN flushAtOnce)
   {
      std::unique_lock<std::mutex> guard(_mutex);

      if (_event.writeOffset < offset)
      {
         _event.writeOffset = offset;
      }

      if (flushAtOnce && _event.flushOffset < offset)
      {
         _event.flushOffset = offset;
      }

      _cv.notify_one();
      return;
   }

   BOOLEAN rlogWriterContext::wait(UINT32 millis, BOOLEAN &timeout, event &e)
   {
      BOOLEAN r = TRUE;
      e.reset();
      timeout = FALSE;

      std::unique_lock<std::mutex> guard(_mutex);
      if (_cv.wait_for(guard, std::chrono::milliseconds(millis),
                       [this]{ return 0 < _event.isActive() || _quit; }))
      {
         if (OSS_UNLIKELY(_quit))
         {
            r = FALSE;
         }
         else
         {
            e = _event;
            _event.reset();
            timeout = FALSE;
         }
      }
      else
      {
         timeout = TRUE;
      }

      return r;
   }

   void rlogWriterContext::attach()
   {
      bool val = _attached.exchange(true);
      SDB_ASSERT(!val, "do not reattach");
   }

   void rlogWriterContext::detach()
   {
      _attached.store(false);
      return;
   }

   void rlogWriterContext::quitWriter()
   {
      if (isWriterAttached())
      {
         std::unique_lock<std::mutex> guard(_mutex);
         _quit = TRUE;
         _cv.notify_one();
         guard.unlock();
         
         while (isWriterAttached())
         {
            ossSleepmillis(1);
         }
      }

      return;
   }
} // namespace vessel

} // namespace engine
