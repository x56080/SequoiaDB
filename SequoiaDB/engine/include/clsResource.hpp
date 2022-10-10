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

   Source File Name = clsResource.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/14/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLS_RESOURCE_HPP__
#define CLS_RESOURCE_HPP__

#include "clsRemoteResource.hpp"
#include "clsStorageResource.hpp"

namespace engine
{

/*
   _clsResource define
 */
class _clsResource : public SDBObject
{
public:
   _clsResource()
   {
   }

public:
   INT32 init( std::unique_ptr< clsStorageResourceAgent > &&agent );

   INT32 init( _netRouteAgent *pAgent,
               pmdOptionsCB *pOptionsCB,
               std::unique_ptr< clsStorageResourceAgent > &&agent,
               _coordDataSourceMgr *pDSMgr = nullptr );

   void fini();

   clsRemoteResource *getCataResource();

   clsStorageResource *getStorageResource();

   void removeCS( const CHAR *csName, BOOLEAN needRemoveRelated = FALSE );

   void removeCS( const CHAR *csName,
                  ossPoolVector< ossPoolString > &subCLs,
                  ossPoolSet< ossPoolString > &mainCLs );

   void removeCL( const CHAR *clFullName );

   void removeCL( const CHAR *clFullName, CoordCataInfoPtr &removedCataPtr );

   void removeCLWithMain( const CHAR *clFullName );

   void removeCLWithMain( const CHAR *clFullName,
                          CoordCataInfoPtr &removedCataPtr,
                          CoordCataInfoPtr &removedMainCataPtr );

   void invalidateAllCLCache();

private:
   clsRemoteResource _cataResource;
   clsStorageResource _storageResource;
};
typedef class _clsResource clsResource;

} // namespace engine

#endif // CLS_RESOURCE_HPP__
