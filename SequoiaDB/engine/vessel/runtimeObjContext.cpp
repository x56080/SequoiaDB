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

   Source File Name = runtimeObjContext.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
      if (_mode.isShared())
      {
         _holder->mutex().release_r();
      }
      else if (_mode.isExclusive())
      {
         _holder->mutex().release_w();
      }
      
      SDB_ASSERT(!_mode.isUpgrade(), "impossible to be upgrade");
      _csid = collectionSpaceId();
      _mode.setNone();
      _holder = nullptr;
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

   collectionContext::~collectionContext()
   {
      if (isOpen())
      {
         close();
      }
   }

   void collectionContext::close()
   {
      if (_mode.isShared())
      {
         _holder->mutex().release_r();
      }
      else if (_mode.isExclusive())
      {
         _holder->mutex().release_w();
      }

      SDB_ASSERT(!_mode.isUpgrade(), "impossible to be upgrade");
      _clid = collectionId();
      _mode.setNone();
      _holder = nullptr;
      
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
