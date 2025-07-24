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

   Source File Name = bufferFlushTask.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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