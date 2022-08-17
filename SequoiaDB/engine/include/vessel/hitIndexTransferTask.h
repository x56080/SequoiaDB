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

   Source File Name = hitIndexTransferTask.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/05/2020  LYC  Initial Draft

   Last Changed =

******************************************************************************/

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