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

   Source File Name = clsResource.cpp

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

#include "clsResource.hpp"
#include "clsTrace.hpp"
#include "pmdEDU.hpp"
#include "msgCatalog.hpp"
#include "msgMessageFormat.hpp"
#include "msgMessage.hpp"
#include "coordRemoteHandle.hpp"
#include "coordRemoteSession.hpp"
#include "coordCommon.hpp"
#include "coordFactory.hpp"
#include "pmd.hpp"
#include "rtnCB.hpp"
#include "rtn.hpp"
#include "coordOmProxy.hpp"
#include "coordSequenceAgent.hpp"
#include "coordDataSource.hpp"
#include "coordGTSAgent.hpp"
#include "../bson/bson.h"
#include "utilArray.hpp"

using namespace bson;

namespace engine
{
// PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRESOURCE__INIT1, "_clsResource::init" )
INT32 _clsResource::init( std::unique_ptr< clsStorageResourceAgent > &&agent )
{
   INT32 rc = SDB_OK;
   PD_TRACE_ENTRY( SDB__CLSRESOURCE__INIT1 );
   rc = _cataResource.init();
   PD_RC_CHECK( rc, PDERROR, "failed to initialize" );
   _storageResource.init( std::move( agent ) );
done:
   PD_TRACE_EXITRC( SDB__CLSRESOURCE__INIT1, rc );
   return rc;
error:
   fini();
   goto done;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRESOURCE__INIT2, "_clsResource::init" )
INT32 _clsResource::init( _netRouteAgent *pAgent,
                          pmdOptionsCB *pOptionsCB,
                          std::unique_ptr< clsStorageResourceAgent > &&agent,
                          _coordDataSourceMgr *pDSMgr )
{
   INT32 rc = SDB_OK;
   PD_TRACE_ENTRY( SDB__CLSRESOURCE__INIT2 );
   rc = _cataResource.init( pAgent, pOptionsCB, pDSMgr );
   PD_RC_CHECK( rc, PDERROR, "failed to initialize" );
   _storageResource.init( std::move( agent ) );
done:
   PD_TRACE_EXITRC( SDB__CLSRESOURCE__INIT2, rc );
   return rc;
error:
   fini();
   goto done;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRESOURCE__FINI, "_clsResource::fini" )
void _clsResource::fini()
{
   PD_TRACE_ENTRY( SDB__CLSRESOURCE__FINI );
   _cataResource.fini();
   _storageResource.fini();
   PD_TRACE_EXIT( SDB__CLSRESOURCE__FINI );
}

clsRemoteResource *_clsResource::getCataResource()
{
   return &_cataResource;
}
clsStorageResource *_clsResource::getStorageResource()
{
   return &_storageResource;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRESOURCE_REMOVECS1, "_clsResource::removeCS" )
void _clsResource::removeCS( const CHAR *csName, BOOLEAN needRemoveRelated )
{
   PD_TRACE_ENTRY( SDB__CLSRESOURCE_REMOVECS1 );
   SDB_ASSERT( nullptr != csName, "can not be nullptr" );
   _cataResource.removeCataInfoByCS( csName, needRemoveRelated );
   _storageResource.removeCSMetaCache( csName );
   PD_TRACE_EXIT( SDB__CLSRESOURCE_REMOVECS1 );
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRESOURCE_REMOVECS2, "_clsResource::removeCS" )
void _clsResource::removeCS( const CHAR *csName,
                             ossPoolVector< ossPoolString > &subCLs,
                             ossPoolSet< ossPoolString > &mainCLs )
{
   PD_TRACE_ENTRY( SDB__CLSRESOURCE_REMOVECS2 );
   SDB_ASSERT( nullptr != csName, "can not be nullptr" );
   _cataResource.removeCataInfoByCS( csName, subCLs, mainCLs );
   _storageResource.removeCSMetaCache( csName );
   PD_TRACE_EXIT( SDB__CLSRESOURCE_REMOVECS2 );
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRESOURCE_REMOVECL1, "_clsResource::removeCL" )
void _clsResource::removeCL( const CHAR *clFullName )
{
   PD_TRACE_ENTRY( SDB__CLSRESOURCE_REMOVECL1 );
   SDB_ASSERT( nullptr != clFullName, "can not be nullptr" );
   _cataResource.removeCataInfo( clFullName );
   _storageResource.removeCLMetaCache( clFullName );
   PD_TRACE_EXIT( SDB__CLSRESOURCE_REMOVECL1 );
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRESOURCE_REMOVECL2, "_clsResource::removeCL" )
void _clsResource::removeCL( const CHAR *clFullName,
                             CoordCataInfoPtr &removedCataPtr )
{
   PD_TRACE_ENTRY( SDB__CLSRESOURCE_REMOVECL2 );
   SDB_ASSERT( nullptr != clFullName, "can not be nullptr" );
   _cataResource.removeCataInfo( clFullName, removedCataPtr );
   _storageResource.removeCLMetaCache( clFullName );
   PD_TRACE_EXIT( SDB__CLSRESOURCE_REMOVECL2 );
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRESOURCE_REMOVECLWITHMAIN1, "_clsResource::removeCLWithMain" )
void _clsResource::removeCLWithMain( const CHAR *clFullName )
{
   PD_TRACE_ENTRY( SDB__CLSRESOURCE_REMOVECLWITHMAIN1 );
   SDB_ASSERT( nullptr != clFullName, "can not be nullptr" );
   _cataResource.removeCataInfoWithMain( clFullName );
   _storageResource.removeCLMetaCache( clFullName );
   PD_TRACE_EXIT( SDB__CLSRESOURCE_REMOVECLWITHMAIN1 );
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRESOURCE_REMOVECLWITHMAIN2, "_clsResource::removeCLWithMain" )
void _clsResource::removeCLWithMain( const CHAR *clFullName,
                                     CoordCataInfoPtr &removedCataPtr,
                                     CoordCataInfoPtr &removedMainCataPtr )
{
   PD_TRACE_ENTRY( SDB__CLSRESOURCE_REMOVECLWITHMAIN2 );
   SDB_ASSERT( nullptr != clFullName, "can not be nullptr" );
   _cataResource.removeCataInfoWithMain(
      clFullName, removedCataPtr, removedMainCataPtr );
   _storageResource.removeCLMetaCache( clFullName );
   PD_TRACE_EXIT( SDB__CLSRESOURCE_REMOVECLWITHMAIN2 );
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRESOURCE_INVALIDATEALLCLCACHE, "_clsResource::invalidateAllCLCache" )
void _clsResource::invalidateAllCLCache()
{
   PD_TRACE_ENTRY( SDB__CLSRESOURCE_INVALIDATEALLCLCACHE );
   _cataResource.invalidateCataInfo();
   _storageResource.invalidateStorageCache();
   PD_TRACE_EXIT( SDB__CLSRESOURCE_INVALIDATEALLCLCACHE );
}
} // namespace engine
