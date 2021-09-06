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

   Source File Name = btreeIndexAccessor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BTREE_INDEX_ACCESSOR_H_
#define VESSEL_BTREE_INDEX_ACCESSOR_H_

#include "vessel/btreeNodePath.h"
#include "ossMemPool.hpp"
#include "vessel/logicalPageBuffer.h"
#include "vessel/btreeNode.h"

namespace engine
{
namespace vessel
{
   class indexContext;
   class requestContext;
   class indexSpace;

   class btreeIndexAccessor : public SDBObject
   {
      public:
         btreeIndexAccessor();
         virtual ~btreeIndexAccessor();
         btreeIndexAccessor(const btreeIndexAccessor &) = delete;
         btreeIndexAccessor &operator=(const btreeIndexAccessor &) = delete;

      public:

      protected:
         INT32 _init(requestContext *context,
                     indexContext *ic);

         void _fini();

         OSS_INLINE BOOLEAN _isInitialized()const
         {
            return NULL != _context;
         }

         requestContext *getContext()
         {
            return _context;
         }
         indexContext *getIndexContext()
         {
            return _ic;
         }
      
      private:
         requestContext *_context = NULL;
         indexContext *_ic = NULL;
         indexSpace *_is = NULL;
         btreeNodePath _path;  
         
         BOOLEAN _checkpointBlocked = FALSE;
   };//class btreeIndexAccessor
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_INDEX_ACCESSOR_H_
