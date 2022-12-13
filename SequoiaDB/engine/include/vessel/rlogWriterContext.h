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

   Source File Name = rlogWriterContext.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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