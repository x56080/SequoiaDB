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

   Source File Name = deltaLogScanner.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/deltaLogScanner.h"
#include "ossLikely.hpp"
#include "vessel/deltaLogUtils.h"

namespace engine
{
namespace vessel
{
   deltaLogScanner::deltaLogScanner()
   {}

   deltaLogScanner::~deltaLogScanner()
   {
      
   }

   void deltaLogScanner::fini()
   {
      _logFiles = NULL;
      _maxOffset = 0;
      _currentOffset = 0;
      return;
   }

   INT32 deltaLogScanner::init(sortedStorageFileList *logFiles,
                               UINT64 minOffset,
                               UINT64 maxBound)
   {
      INT32 rc = SDB_OK;
      const storageFile *file = NULL;
      UINT64 minFileId = 0;
      UINT64 minFileOffset = 0;
      UINT64 maxFileId = 0;
      UINT64 maxFileOffset = 0;

      fini();
      if (OSS_UNLIKELY(NULL == logFiles ||
                       logFiles->isEmpty() ||
                       DPS_INVALID_LSN_OFFSET == minOffset ||
                       maxBound <= minOffset))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _logFiles = logFiles;
      file = _logFiles->getFront();
      
      minFileId = file->getCommonHeadInMem().sequence;
      minFileOffset = minFileId * deltaLogFileDef::MAX_FILE_SIZE;
      if (minOffset < minFileOffset)
      {
         PD_LOG(PDERROR, "current min file offset[%lld] does not match min offset param[%lld]",
                minFileOffset, minOffset);
         rc = SDB_VESSEL_FAILURE_LOADING_DLR;
         goto error;
      }

      file = logFiles->getBack();
      maxFileId = file->getCommonHeadInMem().sequence;
      maxFileOffset = maxFileId * deltaLogFileDef::MAX_FILE_SIZE +
                      file->getSegmentCount() * deltaLogFileDef::FILE_SEGMENT_SIZE;

      if (maxFileOffset <= minOffset)
      {
         PD_LOG(PDERROR, "current max file offset[%lld] does not match min offset param[%lld]",
                maxFileOffset, minOffset);
         rc = SDB_VESSEL_FAILURE_LOADING_DLR;
         goto error;
      }
      if (DPS_INVALID_LSN_OFFSET != maxBound &&
          maxFileOffset < maxBound)
      {
         PD_LOG(PDERROR, "current max file offset[%lld] does not match max bound param[%lld]",
                maxFileOffset, maxBound);
         rc = SDB_VESSEL_FAILURE_LOADING_DLR;
         goto error;
      }

      _maxOffset = (DPS_INVALID_LSN_OFFSET == maxBound) ?
                   maxFileOffset : maxBound;
      _currentOffset = minOffset;
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   INT32 deltaLogScanner::getNext(deltaLogRecord &dlr,
                                  UINT64 *offset)
   {
      INT32 rc = SDB_OK;

      if (NULL == _logFiles)
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (_maxOffset <= (deltaLogFileDef::MIN_RECORD_SIZE_ON_DISK + _currentOffset))
      {
         rc = SDB_VESSEL_EOC;
         goto error;
      }

      dlr.reset();
      rc = getRecord(_currentOffset, dlr);
      if (SDB_OK == rc)
      {
         if (NULL != offset)
         {
            *offset = _currentOffset;
         }
         _currentOffset += dlr.getLogHead()->_size + deltaLogFileDef::CHECKSUM_SIZE;
         _currentOffset = alignDeltaLogRecordOffset(_currentOffset);
      }
      else if (SDB_VESSEL_FAILURE_LOADING_DLR != rc)
      {
         PD_LOG(PDERROR, "failed to get next log record:%d", rc);
         goto error;
      }
      else
      {
         rc = SDB_VESSEL_EOC;
         goto error;
      }
   done:
      return rc;
   error:
      dlr.reset();
      goto done;
   }

   INT32 deltaLogScanner::getRecord(UINT64 offset, deltaLogRecord &dlr)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _logFiles, "can not be null");

      DELTA_LOG_CHECKSUM checksum = 0;
      const DELTA_LOG_CHECKSUM *checksumOnDisk = NULL;
      ossValuePtr segmentPtr = 0;
      UINT32 segmentId = deltaLogFileDef::getInFileSegmentId(offset);
      UINT32 offsetInSegment = deltaLogFileDef::getOffsetInSegment(offset);
      UINT64 fileId = deltaLogFileDef::getLogFileSequenceByOffset(offset);
      const storageFile *file = _logFiles->findFromBackToFront(fileId);
      if (NULL == file)
      {
         PD_LOG(PDERROR, "file with sequence[%lld] does not exist", fileId);
         rc = SDB_FNE;
         goto error;
      }

      rc = file->getSegmentPtr(segmentId, segmentPtr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get segment[%lld, %d] ptr:%d", segmentId, rc);
         goto error;
      }

      SDB_ASSERT((offsetInSegment + deltaLogFileDef::MIN_RECORD_SIZE_ON_DISK) <=
                 deltaLogFileDef::FILE_SEGMENT_SIZE, "impossible");
      dlr.reset((const void *)(segmentPtr + offsetInSegment));
      if (!dlr.isValid())
      {
         rc = SDB_VESSEL_FAILURE_LOADING_DLR;
         goto error;
      }
      else if (deltaLogFileDef::FILE_SEGMENT_SIZE <
               (offsetInSegment + dlr.getLogHead()->_size +
                deltaLogFileDef::CHECKSUM_SIZE))
      {
         rc = SDB_VESSEL_FAILURE_LOADING_DLR;
         goto error;
      }

      checksumOnDisk = (const DELTA_LOG_CHECKSUM *)(segmentPtr
                                                    + offsetInSegment
                                                    + dlr.getLogHead()->_size);
      checksum = createDeltaLogRecordChecksum(dlr);
      if (checksum != *checksumOnDisk)
      {
         rc = SDB_VESSEL_FAILURE_LOADING_DLR;
         goto error;
      }
      
   done:
      return rc;
   error:
      dlr.reset();
      goto done;
   }

}//namespace vessel
}//namespace engine