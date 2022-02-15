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

   Source File Name = runtimeObjContext.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/runtimeObjContext.h"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   collectionSpaceContext::collectionSpaceContext(const collectionSpaceId &csid,
                                                  ossSharedLatchMode mode,
                                                  objectHolder<collectionSpace> *holder):
   _csid(csid),
   _mode(mode),
   _holder(holder)
   {
      SDB_ASSERT(_csid.isValid(), "can not be invalid");
      SDB_ASSERT(_mode.isShared() || _mode.isExclusive(), "can not be invalid");
      SDB_ASSERT(nullptr != _holder && !_holder->isFree(), "can not be invalid");
   }

   collectionSpaceContext::~collectionSpaceContext()
   {
      if (isOpen())
      {
         close();
      }
   }

   collectionSpaceContext::collectionSpaceContext(collectionSpaceContext &&o)
   {
      SDB_ASSERT(o.isOpen(), "can not be invalid");
      _csid = o._csid;
      _mode = o._mode;
      _holder = o._holder;
      o._csid = collectionSpaceId();
      o._mode.setNone();
      o._holder = nullptr;
   }

   collectionSpaceContext &collectionSpaceContext::operator=(collectionSpaceContext &&o)
   {
      SDB_ASSERT(o.isOpen(), "can not be invalid");

      if (isOpen())
      {
         close();
      }
      _csid = o._csid;
      _mode = o._mode;
      _holder = o._holder;
      o._csid = collectionSpaceId();
      o._mode.setNone();
      o._holder = nullptr;
      return *this;
   }

   void collectionSpaceContext::close()
   {
      if (isOpen())
      {
         if (_mode.isShared())
         {
            _holder->getLatch().release_r();
         }
         else if (_mode.isExclusive())
         {
            _holder->getLatch().release_w();
         }
         
         SDB_ASSERT(_mode.isNone(), "impossible to be upgrade");
         _csid = collectionSpaceId();
         _mode.setNone();
         _holder = nullptr;
      }
      return;
   }

//////////////////////collectionContext
   collectionContext::collectionContext(const collectionId &clid,
                                        ossSharedLatchMode mode,
                                        objectHolder<collectionObject> *holder):
   _clid(clid),
   _mode(mode),
   _holder(holder)
   {
      SDB_ASSERT(_clid.isValid(), "can not be invalid");
      SDB_ASSERT(_mode.isShared() || _mode.isExclusive(), "can not be invalid");
      SDB_ASSERT(nullptr != _holder && !_holder->isFree(), "can not be invalid");
   }

   void collectionContext::close()
   {
      if (isOpen())
      {
         if (_mode.isShared())
         {
            _holder->getLatch().release_r();
         }
         else if (_mode.isExclusive())
         {
            _holder->getLatch().release_w();
         }

         SDB_ASSERT(_mode.isNone(), "impossible to be upgrade");
         _clid = collectionId();
         _mode.setNone();
         _holder = nullptr;
      }
      
      return;

   }

   collectionContext::collectionContext(collectionContext &&o):
   _clid(o._clid),
   _mode(o._mode),
   _holder(o._holder)
   {
      SDB_ASSERT(o.isOpen(), "must be open");
      o._clid = collectionId();
      o._mode.setNone();
      o._holder = nullptr;
   }

   collectionContext &collectionContext::operator=(collectionContext &&o)
   {
      SDB_ASSERT(o.isOpen(), "can not be invalid");
      if (isOpen())
      {
         close();
      }

      _clid = o._clid;
      _mode = o._mode;
      _holder = o._holder;

      o._clid = collectionId();
      o._mode.setNone();
      o._holder = nullptr;
      return *this;
   }

} // namespace vessel

} // namespace engine
