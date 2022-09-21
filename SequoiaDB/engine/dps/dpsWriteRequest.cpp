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

   Source File Name = dpsWriteRequest.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "dpsWriteRequest.hpp"
#include "ossLikely.hpp"
#include "pdTrace.hpp"

namespace engine
{
   dpsWriteRequest::~dpsWriteRequest()
   {
      if (nullptr != _bufferOwned)
      {
         SDB_THREAD_FREE(_bufferOwned);
      }
   }

   dpsWriteRequest::dpsWriteRequest(dpsWriteRequest &&o):
   _type(o._type),
   _flags(o._flags),
   _elementNum(o._elementNum),
   _bufferSize(o._bufferSize),
   _buffer(o._buffer),
   _bufferOwned(o._bufferOwned)
   {
      o._bufferOwned = nullptr;
      o.reset();
   }

   dpsWriteRequest &dpsWriteRequest::operator=(dpsWriteRequest &&o)
   {
      if (nullptr != _bufferOwned)
      {
         SDB_THREAD_FREE(_bufferOwned);
         _bufferOwned = nullptr;
      }

      _type = o._type;
      _flags = o._flags;
      _elementNum = o._elementNum;
      _bufferSize = o._bufferSize;
      _buffer = o._buffer;
      _bufferOwned = o._bufferOwned;

      o._bufferOwned = nullptr;
      o.reset();
      return *this;
   }

   void dpsWriteRequest::reset()
   {
      _type = LOG_TYPE_DUMMY;
      _flags = 0;
      _elementNum = 0;
      _bufferSize = 0;
      _buffer = nullptr;
      if (nullptr != _bufferOwned)
      {
         SDB_THREAD_FREE(_bufferOwned);
         _bufferOwned = nullptr;
      }
      return;
   }
} // namespace engine
