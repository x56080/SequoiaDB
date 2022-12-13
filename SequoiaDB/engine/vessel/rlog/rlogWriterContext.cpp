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

   Source File Name = rlogWriterContext.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
