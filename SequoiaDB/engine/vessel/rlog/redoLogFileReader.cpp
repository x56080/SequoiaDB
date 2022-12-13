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

   Source File Name = redoLogFileReader.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/redoLogFileReader.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   INT32 redoLogFileReader::open(const redoLogFile *rfile, UINT64 lsnUpBound)
   {
      INT32 rc = SDB_OK;
      reset();

      if (OSS_UNLIKELY(nullptr == rfile ||
                       !rfile->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (DPS_INVALID_LSN_OFFSET != lsnUpBound &&
               lsnUpBound <= rfile->getStartLSN())
      {
         PD_LOG(PDWARNING, "file start lsn[%lld] hits the bound[%lld]",
                rfile->getStartLSN(), lsnUpBound);
         goto done;
      }

      _rfile = rfile;
      _lsnUpBound = lsnUpBound;

      rc = next();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fetch first record:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      reset();
      goto done;
   }

   void redoLogFileReader::reset()
   {
      _rfile = nullptr;
      _lsnUpBound = 0;
      _offset = 0;
      _header.clear();
      _decompressionBuf.reset();
      return;
   }

   INT32 redoLogFileReader::next()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != _rfile, "can not be invalid");
      if (!_hasNextRecordSpace() || (isReady() && isTailInBound()))
      {
         reset();
      }
      else
      {
         UINT64 currentLSN = _header._lsn;
         UINT64 expectedLSN = 0 == _offset ?
                              _rfile->getStartLSN() : _header._lsn + _header._length;
         rc = _rfile->read(_offset, DPS_LOG_HEAD_SIZE, &_header);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to read data from redo file[%s], rc:%d",
                   _rfile->getPath().c_str(), rc);
            goto error;
         }

         if (0 == _header._length)
         {
            reset();
            goto done;
         }

         /// we did not save any lsn info about prefile,
         /// so skip to validate pre lsn of the first record.
         if ((0 != _offset && currentLSN != _header._preLsn) ||
             expectedLSN != _header._lsn)
         {
            PD_LOG(PDERROR, "failed to extract log record in file[%s:%d]",
                   _rfile->getPath().c_str(), _offset);
            rc = SDB_DPS_CORRUPTED_LOG;
            goto error;
         }
         else
         {
            _offset += _header._length;
            if (OSS_UNLIKELY(_rfile->getFileBodySize() < _offset))
            {
               PD_LOG(PDERROR, "offset[%d] is out of file size after record[%lld] fetched",
                      _offset, _header._lsn);
               rc = SDB_DPS_CORRUPTED_LOG;
               goto error;
            }
         }
      }
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   BOOLEAN redoLogFileReader::isTailInBound() const
   {
      SDB_ASSERT(isReady(), "must be ready");
      return _lsnUpBound <= _header._lsn + _header._length;
   }

   BOOLEAN redoLogFileReader::hasElements() const
   {
      return isReady() && DPS_LOG_HEAD_SIZE < _header._length;
   }

   INT32 redoLogFileReader::getElements(utilUniqueBuffer *outerBuf,
                                        dpsRecordElements &elements) const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != outerBuf, "can not be invalid");
      utilUniqueBuffer *buffer = outerBuf;

      elements.reset();

      if (hasElements())
      {
         SDB_ASSERT(_header._length <= _offset, "impossible");
         UINT32 size = _header._length - DPS_LOG_HEAD_SIZE;
         UINT32 offset = _offset - size;
         if (buffer->getSize() < size)
         {
            rc = buffer->realloc(size);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to reserve buffer:%d", rc);
               goto error;
            }
         }

         rc = _rfile->read(offset, size, buffer->get());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to read file data:%d", rc);
            goto error;
         }

         elements = std::move(dpsRecordElements(buffer->get(), size));
      }

   done:
      return rc;
   error:
      goto done;
   }

} // namespace vessel

} // namespace engine
