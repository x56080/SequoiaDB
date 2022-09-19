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

   Source File Name = clsStorageResource.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/26/2022  ZHY Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CLS_STORAGE_RESOURCE_HPP__
#define CLS_STORAGE_RESOURCE_HPP__

#include "ossLatch.hpp"
#include "pmd.hpp"
#include "utilUniqueID.hpp"
#include "utilStringView.hpp"
#include "dmsCB.hpp"
#include "clsStorageResourceAgent.hpp"
#include <memory>
#include <string>
namespace engine
{
class _clsStorageResource
{
public:
   struct result
   {
      clsIndexInfoSetPtr indexSetPtr = nullptr;
      result(){}
      result( const clsIndexInfoSetPtr &indexSetPtr ) : indexSetPtr( indexSetPtr )
      {
      }
      void reset()
      {
         indexSetPtr.reset();
      }
   };

public:
   _clsStorageResource()
   {
   }

public:
   void init( std::unique_ptr< clsStorageResourceAgent > &&agent );
   void fini();

   INT32 getCLMetaCache( const CHAR *clFullName, result &res );
   INT32 getCLMetaCache( utilCLUniqueID cluid, result &res );
   INT32 getCLIndexSet( const CHAR *clFullName, clsIndexInfoSetPtr &indexSetPtr );
   INT32 getCLIndexSet( utilCLUniqueID cluid, clsIndexInfoSetPtr &indexSetPtr );

   INT32 getOrUpdateCLIndexSet( IExecutor *executor,
                                const CHAR *clFullName,
                                clsIndexInfoSetPtr &indexSetPtr );
   INT32 getOrUpdateCLIndexSet( IExecutor *executor,
                                utilCLUniqueID cluid,
                                clsIndexInfoSetPtr &indexSetPtr );

   void invalidateStorageCache();

   void removeCLMetaCache( const CHAR *clFullName );
   void removeCLMetaCache( utilCLUniqueID cluid );
   void removeCSMetaCache( const CHAR *csName );
   void removeCSMetaCache( utilCSUniqueID csuid );

private:
   UINT32 _getLatchPos( const CHAR *name, UINT32 len );
   UINT32 _getLatchPos( utilCLUniqueID cluid );
   INT32 _updateIndexSet( IExecutor *executor,
                          const CHAR *clFullName,
                          clsIndexInfoSetPtr &indexSetPtr );
   INT32 _updateIndexSet( IExecutor *executor,
                          utilCLUniqueID cluid,
                          clsIndexInfoSetPtr &indexSetPtr );
   void _insert( const clsCLMetaCachePtr &clCataSetPtr );
   void _remove( const CHAR *clFullName );
   void _remove( utilCLUniqueID cluid );
   clsIndexInfoSetPtr _getCLIndexSetWithoutLock( const CHAR *clFullName);
   clsIndexInfoSetPtr _getCLIndexSetWithoutLock( utilCLUniqueID cluid);
private:
   using MAP_NAME_CL = ossPoolMap< utilStringView, clsCLMetaCachePtr >;
   using MAP_CLUID_CL = ossPoolMap< utilCLUniqueID, clsCLMetaCachePtr >; 

private:
   MAP_NAME_CL _mapNameToCL;
   MAP_CLUID_CL _mapUidToCL;
   ossSpinSLatch _mapLatch;

   static constexpr INT32 LATCH_COUNT = 32;
   ossSpinXLatch _latches[ LATCH_COUNT ];

   std::unique_ptr< clsStorageResourceAgent > _agent;
};
using clsStorageResource = _clsStorageResource;
} // namespace engine

#endif