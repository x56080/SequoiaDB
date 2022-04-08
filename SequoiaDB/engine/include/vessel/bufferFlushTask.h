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

   Source File Name = bufferFlushTask.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BUFFER_FLUSH_TASK_H_
#define VESSEL_BUFFER_FLUSH_TASK_H_

#include "vessel/globalPageID.h"
#include "vessel/slice.h"

namespace engine
{
namespace vessel
{
   class bufferFlushTask : public SDBObject
   { 
      public:
         bufferFlushTask(){}
         bufferFlushTask(const globalPageID &gpid,
                         const CHAR *buffer):
         _gpid(gpid),
         _buffer(buffer){}

      public:
         struct comp
         {
            OSS_INLINE BOOLEAN operator()(const bufferFlushTask &l,
                                          const bufferFlushTask &r)const
            {
               return l._gpid < r._gpid;
            }
         };

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return _gpid.isValid() && nullptr != _buffer;
         }
         OSS_INLINE const globalPageID &gpid()const {return _gpid;}
         OSS_INLINE const CHAR *getBuffer()const {return _buffer;}

      private:
         globalPageID _gpid;
         const CHAR *_buffer = nullptr;
   };//class bufferFlushTask
} // namespace vessel

} // namespace engine


#endif//VESSEL_BUFFER_FLUSH_TASK_H_