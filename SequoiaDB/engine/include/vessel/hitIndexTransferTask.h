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

   Source File Name = hitIndexTransferTask.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/05/2020  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_HIT_INDEX_TRANSFER_TASK_H_
#define VESSEL_HIT_INDEX_TRANSFER_TASK_H_

#include "vessel/globalIndexID.h"
#include "rocksdb/sst_file_reader.h"

namespace engine
{
namespace vessel
{
   class hitIndexTransferTask : public SDBObject
   {
      public:
         hitIndexTransferTask() = default;
         ~hitIndexTransferTask() = default;
         explicit hitIndexTransferTask(UINT32 taskId,
                                       rocksdb::SstFileReader *reader,
                                       const globalIndexID &id):
         _taskId(taskId),
         _reader(reader),
         _id(id)
         {}

      public:
         OSS_INLINE BOOLEAN isValid() const
         {
            return nullptr != _reader && _id.isValid();
         }
         OSS_INLINE rocksdb::SstFileReader *getReader() const {return _reader;}
         OSS_INLINE const globalIndexID &getGlobalIndexID() const {return _id;}
         OSS_INLINE UINT32 getTaskId()const {return _taskId;}

         OSS_INLINE void reset()
         {
            _taskId = 0;
            _reader = nullptr;
            _id.reset();
         }

      private:
         UINT32 _taskId = 0;
         rocksdb::SstFileReader *_reader = nullptr;
         globalIndexID _id;
         
   };//class hitIndexTransferTask
} // namespace vessel
} // namespace engine

#endif // VESSEL_HIT_INDEX_TRANSFER_TASK_H_