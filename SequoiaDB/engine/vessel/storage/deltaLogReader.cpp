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

   Source File Name = deltaLogReader.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/deltaLogReader.h"
#include "ossLikely.hpp"
#include "vessel/deltaLogUtils.h"

namespace engine
{
namespace vessel
{
   deltaLogReader::deltaLogReader()
   {}

   deltaLogReader::~deltaLogReader()
   {
      fini();
   }

   void deltaLogReader::fini()
   {
      _logFiles = NULL;
      _firstRecordOffset = DPS_INVALID_LSN_OFFSET;
      _lastRecordHeadOffset = DPS_INVALID_LSN_OFFSET;
      _maxFileOffset = 0;
      _currentOffset = 0;
      return;
   }

   INT32 deltaLogReader::init(const storageFileMap *logFiles,
                              UINT64 firstRecordOffset,
                              UINT64 lastRecordHeadOffset)
   {
      INT32 rc = SDB_OK;
      const storageFile *file = NULL;
      UINT64 minFileId = 0;
      UINT64 maxFileId = 0;
      UINT64 minFileOffset = 0;

      fini();
      if (OSS_UNLIKELY(NULL == logFiles ||
                       logFiles->isEmpty() ||
                       DPS_INVALID_LSN_OFFSET == firstRecordOffset ||
                       lastRecordHeadOffset < firstRecordOffset))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      file = logFiles->getFirst();
      if (NULL == file || !file->isOpen())
      {
         PD_LOG(PDERROR, "invalid file found");
         rc = SDB_INVALIDARG;
         goto error;
      }
      minFileId = file->getCommonHeadInMem().sequence;
      minFileOffset = minFileId * deltaLogFileDef::MAX_FILE_SIZE;

      file = logFiles->getLast();
      if (NULL == file || !file->isOpen())
      {
         PD_LOG(PDERROR, "invalid file found");
         rc = SDB_INVALIDARG;
         goto error;
      }
      maxFileId = file->getCommonHeadInMem().sequence;
      _maxFileOffset = (file->getCommonHeadInMem().sequence * deltaLogFileDef::MAX_FILE_SIZE)
                       + (file->getSegmentCount() * deltaLogFileDef::FILE_SEGMENT_SIZE);

      /// valid offset is [minFileOffset, _maxFileOffset)
      if (firstRecordOffset < minFileOffset ||
          _maxFileOffset <= firstRecordOffset)
      {
         PD_LOG(PDERROR, "the first record offset[%lld] is not in valid range[%lld, %lld)",
                firstRecordOffset, minFileOffset, _maxFileOffset);
         rc = SDB_DPS_LSN_OUTOFRANGE;
         goto error;
      }

      if (DPS_INVALID_LSN_OFFSET != lastRecordHeadOffset)
      {
         if (lastRecordHeadOffset < minFileOffset ||
             _maxFileOffset <= lastRecordHeadOffset)
         {
            PD_LOG(PDERROR, "the last record head offset[%lld] is not in valid range[%lld, %lld)",
                lastRecordHeadOffset, minFileOffset, _maxFileOffset);
            rc = SDB_DPS_LSN_OUTOFRANGE;
            goto error;
         }
      }

      _logFiles = logFiles;
      _firstRecordOffset = firstRecordOffset;
      _lastRecordHeadOffset = lastRecordHeadOffset;
      _currentOffset = _firstRecordOffset; 
      
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   INT32 deltaLogReader::getNext(deltaLogRecord &dlr,
                                 UINT64 *offset)
   {
      INT32 rc = SDB_OK;

      if (DPS_INVALID_LSN_OFFSET == _firstRecordOffset)
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (_lastRecordHeadOffset < _currentOffset)
      {
         rc = SDB_VESSEL_EOC;
         goto error;
      }
      else if (_maxFileOffset <= _currentOffset)
      {
         rc = SDB_VESSEL_EOC;
         goto error;
      }

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
      else if (DPS_INVALID_LSN_OFFSET != _lastRecordHeadOffset)
      {
         PD_LOG(PDERROR, "failed to get log record at offset[%lld]", _currentOffset);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      else
      {
         /// If _lastRecordHeadOffset not set, stop where first failure reading. 
         rc = SDB_VESSEL_EOC;
         goto error;
      }
   done:
      return rc;
   error:
      dlr.reset();
      goto done;
   }

   INT32 deltaLogReader::getRecord(UINT64 offset, deltaLogRecord &dlr)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _logFiles, "can not be null");
      SDB_ASSERT(_currentOffset <= _lastRecordHeadOffset, "impossible");

      DELTA_LOG_CHECKSUM checksum = 0;
      const DELTA_LOG_CHECKSUM *checksumOnDisk = NULL;
      ossValuePtr segmentPtr = 0;
      UINT32 segmentId = deltaLogFileDef::getSegmentIdInFileByOffset(offset);
      UINT32 offsetInSegment = deltaLogFileDef::getOffsetInSegmentByOffset(offset);
      UINT64 fileId = deltaLogFileDef::getLogFileSequenceByOffset(offset);
      const storageFile *file = _logFiles->get(fileId);
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