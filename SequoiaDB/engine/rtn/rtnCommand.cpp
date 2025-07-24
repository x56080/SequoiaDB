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

   Source File Name = rtnCommand.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          13/12/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnCommand.hpp"
#include "pd.hpp"
#include "pmdEDU.hpp"
#include "rtn.hpp"
#include "dms.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "rtnContext.hpp"
#include "dmsStorageUnit.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
#include "monDump.hpp"
#include "ossMem.h"
#include "rtnContextListLob.hpp"
#include "rtnSessionProperty.hpp"
#include "aggrDef.hpp"
#include "utilCompressor.hpp"
#include "msgMessageFormat.hpp"

#if defined (_DEBUG)
// for qgmDebugQuery function
#endif

using namespace bson ;
using namespace std ;

namespace engine
{
   _rtnCommand::_rtnCommand ()
   {
      _fromService = 0 ;
   }

   _rtnCommand::~_rtnCommand ()
   {
   }

   void _rtnCommand::setFromService( INT32 fromService )
   {
      _fromService = fromService ;
   }

   INT32 _rtnCommand::spaceNode()
   {
      return CMD_SPACE_NODE_ALL ;
   }

   INT32 _rtnCommand::spaceService ()
   {
      return CMD_SPACE_SERVICE_ALL ;
   }

   BOOLEAN _rtnCommand::writable ()
   {
      return FALSE ;
   }

   const CHAR *_rtnCommand::collectionFullName ()
   {
      return NULL ;
   }

   _rtnCmdBuilder::_rtnCmdBuilder ()
   {
      _pCmdInfoRoot = NULL ;
   }

   _rtnCmdBuilder::~_rtnCmdBuilder ()
   {
      if ( _pCmdInfoRoot )
      {
         _releaseCmdInfo ( _pCmdInfoRoot ) ;
         _pCmdInfoRoot = NULL ;
      }
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCMDBUILDER_RELCMDINFO, "_rtnCmdBuilder::_releaseCmdInfo" )
   void _rtnCmdBuilder::_releaseCmdInfo ( _cmdBuilderInfo *pCmdInfo )
   {
      PD_TRACE_ENTRY ( SDB__RTNCMDBUILDER_RELCMDINFO ) ;
      if ( pCmdInfo->next )
      {
         _releaseCmdInfo ( pCmdInfo->next ) ;
         pCmdInfo->next = NULL ;
      }
      if ( pCmdInfo->sub )
      {
         _releaseCmdInfo ( pCmdInfo->sub ) ;
         pCmdInfo->sub = NULL ;
      }

      SDB_OSS_DEL pCmdInfo ;
      PD_TRACE_EXIT ( SDB__RTNCMDBUILDER_RELCMDINFO ) ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCMDBUILDER__REGISTER, "_rtnCmdBuilder::_register" )
   INT32 _rtnCmdBuilder::_register ( const CHAR * name, CDM_NEW_FUNC pFunc )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNCMDBUILDER__REGISTER ) ;

      if ( !_pCmdInfoRoot )
      {
         _pCmdInfoRoot = SDB_OSS_NEW _cmdBuilderInfo ;
         _pCmdInfoRoot->cmdName = name ;
         _pCmdInfoRoot->createFunc = pFunc ;
         _pCmdInfoRoot->nameSize = ossStrlen ( name ) ;
         _pCmdInfoRoot->next = NULL ;
         _pCmdInfoRoot->sub = NULL ;
      }
      else
      {
         rc = _insert ( _pCmdInfoRoot, name, pFunc ) ;
      }

      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Register command [%s] failed", name ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__RTNCMDBUILDER__REGISTER, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCMDBUILDER__INSERT, "_rtnCmdBuilder::_insert" )
   INT32 _rtnCmdBuilder::_insert ( _cmdBuilderInfo * pCmdInfo,
                                   const CHAR * name, CDM_NEW_FUNC pFunc )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNCMDBUILDER__INSERT ) ;
      _cmdBuilderInfo *newCmdInfo = NULL ;
      UINT32 sameNum = _near ( pCmdInfo->cmdName.c_str(), name ) ;

      if ( sameNum == 0 )
      {
         if ( !pCmdInfo->sub )
         {
            newCmdInfo = SDB_OSS_NEW _cmdBuilderInfo ;
            newCmdInfo->cmdName = name ;
            newCmdInfo->createFunc = pFunc ;
            newCmdInfo->nameSize = ossStrlen(name) ;
            newCmdInfo->next = NULL ;
            newCmdInfo->sub = NULL ;

            pCmdInfo->sub = newCmdInfo ;
         }
         else
         {
            rc = _insert ( pCmdInfo->sub, name, pFunc ) ;
         }
      }
      else if ( sameNum == pCmdInfo->nameSize )
      {
         if ( sameNum == ossStrlen ( name ) )
         {
            if ( !pCmdInfo->createFunc )
            {
               pCmdInfo->createFunc = pFunc ;
            }
            else
            {
               rc = SDB_SYS ; //already exist
            }
         }
         else if ( !pCmdInfo->next )
         {
            newCmdInfo = SDB_OSS_NEW _cmdBuilderInfo ;
            newCmdInfo->cmdName = &name[sameNum] ;
            newCmdInfo->createFunc = pFunc ;
            newCmdInfo->nameSize = ossStrlen(&name[sameNum]) ;
            newCmdInfo->next = NULL ;
            newCmdInfo->sub = NULL ;

            pCmdInfo->next = newCmdInfo ;
         }
         else
         {
            rc = _insert ( pCmdInfo->next, &name[sameNum], pFunc ) ;
         }
      }
      else
      {
         //split next node first
         newCmdInfo = SDB_OSS_NEW _cmdBuilderInfo ;
         newCmdInfo->cmdName = pCmdInfo->cmdName.substr( sameNum ) ;
         newCmdInfo->createFunc = pCmdInfo->createFunc ;
         newCmdInfo->nameSize = pCmdInfo->nameSize - sameNum ;
         newCmdInfo->next = pCmdInfo->next ;
         newCmdInfo->sub = NULL ;

         pCmdInfo->next = newCmdInfo ;

         //change cur node
         pCmdInfo->cmdName = pCmdInfo->cmdName.substr ( 0, sameNum ) ;
         pCmdInfo->nameSize = sameNum ;

         if ( sameNum != ossStrlen ( name ) )
         {
            pCmdInfo->createFunc = NULL ;

            rc = _insert ( newCmdInfo, &name[sameNum], pFunc ) ;
         }
         else
         {
            pCmdInfo->createFunc = pFunc ;
         }
      }

      PD_TRACE_EXITRC ( SDB__RTNCMDBUILDER__INSERT, rc ) ;
      return rc ;
   }

   _rtnCommand *_rtnCmdBuilder::create ( const CHAR *command )
   {
      CDM_NEW_FUNC pFunc = _find ( command ) ;
      if ( pFunc )
      {
         return (*pFunc)() ;
      }

      return NULL ;
   }

   void _rtnCmdBuilder::release ( _rtnCommand *pCommand )
   {
      if ( pCommand )
      {
         SDB_OSS_DEL pCommand ;
      }
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCMDBUILDER__FIND, "_rtnCmdBuilder::_find" )
   CDM_NEW_FUNC _rtnCmdBuilder::_find ( const CHAR * name )
   {
      PD_TRACE_ENTRY ( SDB__RTNCMDBUILDER__FIND ) ;
      CDM_NEW_FUNC pFunc = NULL ;
      _cmdBuilderInfo *pCmdNode = _pCmdInfoRoot ;
      UINT32 index = 0 ;
      BOOLEAN findRoot = FALSE ;

      while ( pCmdNode )
      {
         findRoot = FALSE ;
         if ( pCmdNode->cmdName.at ( 0 ) != name[index] )
         {
            pCmdNode = pCmdNode->sub ;
         }
         else
         {
            findRoot = TRUE ;
         }

         if ( findRoot )
         {
            if ( _near ( pCmdNode->cmdName.c_str(), &name[index] )
               != pCmdNode->nameSize )
            {
               break ;
            }
            index += pCmdNode->nameSize ;
            if ( name[index] == 0 ) //find
            {
               pFunc = pCmdNode->createFunc ;
               break ;
            }

            pCmdNode = pCmdNode->next ;
         }
      }

      PD_TRACE_EXIT ( SDB__RTNCMDBUILDER__FIND ) ;
      return pFunc ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCMDBUILDER__NEAR, "_rtnCmdBuilder::_near" )
   UINT32 _rtnCmdBuilder::_near ( const CHAR *str1, const CHAR *str2 )
   {
      PD_TRACE_ENTRY ( SDB__RTNCMDBUILDER__NEAR ) ;
      UINT32 same = 0 ;
      while ( str1[same] && str2[same] && str1[same] == str2[same] )
      {
         ++same ;
      }

      PD_TRACE_EXIT ( SDB__RTNCMDBUILDER__NEAR ) ;
      return same ;
   }

   _rtnCmdBuilder *getRtnCmdBuilder ()
   {
      static _rtnCmdBuilder cmdBuilder ;
      return &cmdBuilder ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCMDASSIT__RTNCMDASSIT, "_rtnCmdAssit::_rtnCmdAssit" )
   _rtnCmdAssit::_rtnCmdAssit ( CDM_NEW_FUNC pFunc )
   {
      PD_TRACE_ENTRY ( SDB__RTNCMDASSIT__RTNCMDASSIT ) ;
      if ( pFunc )
      {
         _rtnCommand *pCommand = (*pFunc)() ;
         if ( pCommand )
         {
            getRtnCmdBuilder()->_register ( pCommand->name(), pFunc ) ;
            SDB_OSS_DEL pCommand ;
            pCommand = NULL ;
         }
      }
      PD_TRACE_EXIT ( SDB__RTNCMDASSIT__RTNCMDASSIT ) ;
   }

   _rtnCmdAssit::~_rtnCmdAssit ()
   {
   }

   //Command list:
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnCreateGroup)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnRemoveGroup)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnCreateNode)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnRemoveNode)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnUpdateNode)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnActiveGroup)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnStartNode)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnShutdownNode)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnShutdownGroup)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnGetConfig)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListDomains)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListGroups)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListProcedures)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnCreateProcedure)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnRemoveProcedure)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListCSInDomain)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListCLInDomain)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnCreateCataGroup)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnCreateDomain)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnDropDomain)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnAlterDomain)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnAddDomainGroup)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnRemoveDomainGroup)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotCata )
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotCataIntr)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnWaitTask)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListTask)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListUsers)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnGetDCInfo)

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnBackup)
   _rtnBackup::_rtnBackup ()
      :_matherBuff ( NULL )
   {
      _backupName       = NULL ;
      _path             = NULL ;
      _desp             = NULL ;
      _ensureInc        = FALSE ;
      _rewrite          = FALSE ;
   }

   _rtnBackup::~_rtnBackup ()
   {
   }

   const CHAR *_rtnBackup::name ()
   {
      return NAME_BACKUP_OFFLINE ;
   }

   RTN_COMMAND_TYPE _rtnBackup::type ()
   {
      return CMD_BACKUP_OFFLINE ;
   }

   INT32 _rtnBackup::init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                            const CHAR * pMatcherBuff, const CHAR * pSelectBuff,
                            const CHAR * pOrderByBuff, const CHAR * pHintBuff )
   {
      INT32 rc = SDB_OK ;
      _matherBuff = pMatcherBuff ;

      if ( !_matherBuff )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         BSONObj matcher( _matherBuff ) ;

         rc = rtnGetStringElement( matcher, FIELD_NAME_NAME, &_backupName ) ;
         PD_CHECK( SDB_OK == rc || SDB_FIELD_NOT_EXIST == rc, rc, error,
                   PDWARNING, "Get field[%s] failed, rc: %d", FIELD_NAME_NAME,
                   rc ) ;
         rc = SDB_OK ;

         rc = rtnGetStringElement( matcher, FIELD_NAME_PATH, &_path ) ;
         PD_CHECK( SDB_OK == rc || SDB_FIELD_NOT_EXIST == rc, rc, error,
                   PDWARNING, "Get field[%s] failed, rc: %d", FIELD_NAME_PATH,
                   rc ) ;
         rc = SDB_OK ;

         rc = rtnGetStringElement( matcher, FIELD_NAME_DESP, &_desp ) ;
         PD_CHECK( SDB_OK == rc || SDB_FIELD_NOT_EXIST == rc, rc, error,
                   PDWARNING, "Get field[%s] failed, rc: %d", FIELD_NAME_DESP,
                   rc ) ;
         rc = SDB_OK ;

         rc = rtnGetBooleanElement( matcher, FIELD_NAME_ENSURE_INC,
                                    _ensureInc ) ;
         PD_CHECK( SDB_OK == rc || SDB_FIELD_NOT_EXIST == rc, rc, error,
                   PDWARNING, "Get field[%s] failed, rc: %d",
                   FIELD_NAME_ENSURE_INC, rc ) ;
         rc = SDB_OK ;

         rc = rtnGetBooleanElement( matcher, FIELD_NAME_OVERWRITE, _rewrite ) ;
         PD_CHECK( SDB_OK == rc || SDB_FIELD_NOT_EXIST == rc, rc, error,
                   PDWARNING, "Get field[%s] failed, rc: %d",
                   FIELD_NAME_OVERWRITE, rc ) ;
         rc = SDB_OK ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnBackup::doit ( _pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                            SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                            INT16 w , INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObj matcher( _matherBuff ) ;
         rc = rtnBackup( cb, _path, _backupName, _ensureInc, _rewrite,
                         _desp, matcher ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what () ) ;
         rc = SDB_SYS ;
      }
      return rc ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnCreateCollection)
   _rtnCreateCollection::_rtnCreateCollection ()
   :_collectionName ( NULL ),
    _attributes(0),
    _compressorType(UTIL_COMPRESSOR_INVALID)
   {
   }

   _rtnCreateCollection::~_rtnCreateCollection ()
   {
   }

   const CHAR *_rtnCreateCollection::name()
   {
      return NAME_CREATE_COLLECTION ;
   }

   RTN_COMMAND_TYPE _rtnCreateCollection::type()
   {
      return CMD_CREATE_COLLECTION ;
   }

   BOOLEAN _rtnCreateCollection::writable()
   {
      return TRUE ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCREATECL_INIT, "_rtnCreateCollection::init" )
   INT32 _rtnCreateCollection::init( INT32 flags, INT64 numToSkip,
                                     INT64 numToReturn,
                                     const CHAR * pMatcherBuff,
                                     const CHAR * pSelectBuff,
                                     const CHAR * pOrderByBuff,
                                     const CHAR * pHintBuff)
   {
      INT32 rc = SDB_OK ;
      BOOLEAN enSureIndex = TRUE ;
      BOOLEAN isCompressed = FALSE ;
      BOOLEAN hasCompressed = TRUE ;
      BOOLEAN hasCompressType = TRUE ;
      BOOLEAN autoIndexId = TRUE ;
      const CHAR *compressionType = NULL ;
      PD_TRACE_ENTRY ( SDB__RTNCREATECL_INIT ) ;
      BSONObj matcher ( pMatcherBuff ) ;
      BSONElement ele ;
      rc = rtnGetStringElement ( matcher, FIELD_NAME_NAME,
                                 &_collectionName ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to extract %s field for collection "
                  "creation, rc = %d", FIELD_NAME_NAME, rc ) ;
         goto error ;
      }
      // ensure sharding key
      rc = rtnGetBooleanElement( matcher, FIELD_NAME_ENSURE_SHDINDEX,
                                 enSureIndex ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_OK ;
         enSureIndex = TRUE ;
      }
      PD_RC_CHECK( rc, PDERROR, "Field[%s] value is error in obj[%s]",
                   FIELD_NAME_ENSURE_SHDINDEX, matcher.toString().c_str() ) ;
      // if we want to create sharding key index, let's do it
      if ( enSureIndex )
      {
         rc = rtnGetObjElement ( matcher, FIELD_NAME_SHARDINGKEY,
                                 _shardingKey ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR, "Field[%s] value is error in obj[%s]",
                      FIELD_NAME_SHARDINGKEY,
                      matcher.toString().c_str() ) ;
      }
      // check compress
      rc = rtnGetBooleanElement ( matcher, FIELD_NAME_COMPRESSED,
                                  isCompressed ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         hasCompressed = FALSE ;
      }
      rc = rtnGetStringElement( matcher, FIELD_NAME_COMPRESSIONTYPE,
                                &compressionType ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         hasCompressType = FALSE ;
      }
      else if ( rc )
      {
         PD_LOG( PDERROR, "Failed to get CompressionType, rc: %d", rc ) ;
         goto error ;
      }
      else
      {
         if ( 0 == ossStrcmp( compressionType, VALUE_NAME_LZW ) )
         {
            _compressorType = UTIL_COMPRESSOR_LZW ;
         }
         else if ( 0 == ossStrcmp( compressionType, VALUE_NAME_SNAPPY ) )
         {
            _compressorType = UTIL_COMPRESSOR_SNAPPY ;
         }
         else
         {
            PD_LOG( PDERROR, "Compression type[%s] is invalid",
                    compressionType ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }

      if ( isCompressed && !hasCompressType )
      {
         _compressorType = UTIL_COMPRESSOR_LZW ;
      }
      if ( !hasCompressed && hasCompressType )
      {
         isCompressed = TRUE ;
      }
      if ( !isCompressed && hasCompressType )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR,
                 "CompressionType can't be set when Compressed is false" ) ;
         goto error ;
      }
      if ( !hasCompressed && !hasCompressType )
      {
         isCompressed = TRUE ;
         _compressorType = UTIL_COMPRESSOR_LZW ;
      }
      if ( isCompressed )
      {
         _attributes |= DMS_MB_ATTR_COMPRESSED ;
      }

      /// auto index id
      rc = rtnGetBooleanElement( matcher, FIELD_NAME_AUTO_INDEX_ID,
                                 autoIndexId ) ;
      if ( SDB_OK == rc && !autoIndexId )
      {
         _attributes |= DMS_MB_ATTR_NOIDINDEX ;
      }
      rc = SDB_OK ;
   done :
      PD_TRACE_EXITRC ( SDB__RTNCREATECL_INIT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   const CHAR *_rtnCreateCollection::collectionFullName ()
   {
      return _collectionName ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCREATECL_DOIT, "_rtnCreateCollection::doit" )
   INT32 _rtnCreateCollection::doit ( _pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                                      SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                                      INT16 w , INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNCREATECL_DOIT ) ;

      rc = rtnCreateCollectionCommand ( _collectionName, _shardingKey,
                                        _attributes, cb, dmsCB, dpsCB,
                                        _compressorType ) ;

      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         CHAR szTmp[ 50 ] = { 0 } ;
         mbAttr2String( _attributes, szTmp, sizeof(szTmp) - 1 ) ;
         /// AUDIT
         PD_AUDIT_COMMAND( AUDIT_DDL, name(), AUDIT_OBJ_CL,
                           _collectionName, rc,
                           "ShardingKey:%s, Attribute:0x%08x(%s), "
                           "CompressType:%d(%s)",
                           _shardingKey.toString().c_str(),
                           _attributes, szTmp,
                           _compressorType,
                           utilCompressType2String( _compressorType ) ) ;
      }

      PD_TRACE_EXITRC ( SDB__RTNCREATECL_DOIT, rc ) ;
      return rc ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnCreateCollectionspace)
   _rtnCreateCollectionspace::_rtnCreateCollectionspace ()
   :_spaceName ( NULL ),
    _pageSize( 0 ),
    _lobPageSize( 0 )
   {

   }

   _rtnCreateCollectionspace::~_rtnCreateCollectionspace ()
   {
   }

   const CHAR *_rtnCreateCollectionspace::name ()
   {
      return NAME_CREATE_COLLECTIONSPACE ;
   }

   RTN_COMMAND_TYPE _rtnCreateCollectionspace::type ()
   {
      return CMD_CREATE_COLLECTIONSPACE ;
   }

   BOOLEAN _rtnCreateCollectionspace::writable ()
   {
      return TRUE ;
   }

   INT32 _rtnCreateCollectionspace::init ( INT32 flags, INT64 numToSkip,
                                           INT64 numToReturn,
                                           const CHAR * pMatcherBuff,
                                           const CHAR * pSelectBuff,
                                           const CHAR * pOrderByBuff,
                                           const CHAR * pHintBuff)
   {
      BSONObj mather ( pMatcherBuff ) ;
      INT32 rc = rtnGetIntElement ( mather, FIELD_NAME_PAGE_SIZE,
                                    _pageSize ) ;
      if ( SDB_OK != rc )
      {
         _pageSize = DMS_PAGE_SIZE_DFT ;
      }

      rc = rtnGetIntElement( mather, FIELD_NAME_LOB_PAGE_SIZE,
                             _lobPageSize ) ;
      if ( SDB_OK != rc )
      {
         _lobPageSize = DMS_DEFAULT_LOB_PAGE_SZ ;
      }

      return rtnGetStringElement ( mather, FIELD_NAME_NAME, &_spaceName ) ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCREATECS_DOIT, "_rtnCreateCollectionspace::doit" )
   INT32 _rtnCreateCollectionspace::doit ( _pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                                          SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                                          INT16 w , INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNCREATECS_DOIT ) ;

      rc = rtnCreateCollectionSpaceCommand ( _spaceName, cb, dmsCB, dpsCB,
                                             _pageSize, _lobPageSize ) ;

      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         /// AUDIT
         PD_AUDIT_COMMAND( AUDIT_DDL, name(), AUDIT_OBJ_CS,
                           _spaceName, rc,
                           "PageSize:%u, LobPageSize:%u",
                           _pageSize, _lobPageSize ) ;
      }

      PD_TRACE_EXITRC ( SDB__RTNCREATECS_DOIT, rc ) ;
      return rc ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnCreateIndex)

   _rtnCreateIndex::_rtnCreateIndex ()
   : _collectionName ( NULL ),
     _sortBufferSize ( SDB_INDEX_SORT_BUFFER_DEFAULT_SIZE )
   {
   }

   _rtnCreateIndex::~_rtnCreateIndex ()
   {
   }

   const CHAR *_rtnCreateIndex::name ()
   {
      return NAME_CREATE_INDEX ;
   }

   RTN_COMMAND_TYPE _rtnCreateIndex::type ()
   {
      return CMD_CREATE_INDEX ;
   }

   BOOLEAN _rtnCreateIndex::writable ()
   {
      return TRUE ;
   }

   const CHAR *_rtnCreateIndex::collectionFullName ()
   {
      return _collectionName ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCREATEINDEX_INIT, "_rtnCreateIndex::init" )
   INT32 _rtnCreateIndex::init ( INT32 flags, INT64 numToSkip,
                                 INT64 numToReturn,
                                 const CHAR * pMatcherBuff,
                                 const CHAR * pSelectBuff,
                                 const CHAR * pOrderByBuff,
                                 const CHAR * pHintBuff)
   {
      PD_TRACE_ENTRY ( SDB__RTNCREATEINDEX_INIT ) ;
      BSONObj arg ( pMatcherBuff ) ;
      BSONObj hint ( pHintBuff ) ;

      INT32 rc = rtnGetStringElement ( arg, FIELD_NAME_COLLECTION,
                                       &_collectionName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to get string [collection] " ) ;
         goto error ;
      }

      rc = rtnGetObjElement ( arg, FIELD_NAME_INDEX, _index ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to get object index " ) ;
         goto error ;
      }

      if ( hint.hasField( IXM_FIELD_NAME_SORT_BUFFER_SIZE ) )
      {
         rc = rtnGetIntElement( hint, IXM_FIELD_NAME_SORT_BUFFER_SIZE,
                                _sortBufferSize ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Failed to get index sort buffer, hint: %s",
                     hint.toString().c_str() ) ;
            goto error ;
         }

         if ( _sortBufferSize < 0 )
         {
            PD_LOG ( PDERROR, "invalid index sort buffer size: %d", _sortBufferSize ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB__RTNCREATEINDEX_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCREATEINDEX_DOIT, "_rtnCreateIndex::doit" )
   INT32 _rtnCreateIndex::doit ( _pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                                 SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                                 INT16 w , INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNCREATEINDEX_DOIT ) ;

      BOOLEAN isSys = FALSE ;
      if ( !pmdGetOptionCB()->authEnabled() )
      {
         isSys = TRUE ;
      }

      rc = rtnCreateIndexCommand ( _collectionName, _index, cb,
                                   dmsCB, dpsCB, isSys, _sortBufferSize ) ;

      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         /// AUDIT
         PD_AUDIT_COMMAND( AUDIT_DDL, name(), AUDIT_OBJ_CL,
                           _collectionName, rc,
                           "IndexDef:%s, SortBuffSize:%d",
                           _index.toString().c_str(), _sortBufferSize ) ;
      }
      PD_TRACE_EXITRC ( SDB__RTNCREATEINDEX_DOIT, rc ) ;
      return rc ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnDropCollection)

   _rtnDropCollection::_rtnDropCollection ()
   :_collectionName ( NULL )
   {
   }

   _rtnDropCollection::~_rtnDropCollection ()
   {
   }

   const CHAR *_rtnDropCollection::name ()
   {
      return NAME_DROP_COLLECTION ;
   }

   RTN_COMMAND_TYPE _rtnDropCollection::type ()
   {
      return CMD_DROP_COLLECTION ;
   }

   BOOLEAN _rtnDropCollection::writable ()
   {
      return TRUE ;
   }

   const CHAR *_rtnDropCollection::collectionFullName ()
   {
      return _collectionName ;
   }

   INT32 _rtnDropCollection::init ( INT32 flags, INT64 numToSkip,
                                    INT64 numToReturn,
                                    const CHAR * pMatcherBuff,
                                    const CHAR * pSelectBuff,
                                    const CHAR * pOrderByBuff,
                                    const CHAR * pHintBuff )
   {
      BSONObj arg ( pMatcherBuff ) ;
      return rtnGetStringElement ( arg, FIELD_NAME_NAME, &_collectionName ) ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDROPCL_DOIT, "" )
   INT32 _rtnDropCollection::doit ( _pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                                    SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                                    INT16 w , INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNDROPCL_DOIT ) ;
      SDB_ASSERT( cb, "cb can't be null!");
      SDB_ASSERT( rtnCB, "rtnCB can't be null!");
      SDB_ASSERT( pContextID, "pContextID can't be null!");
      *pContextID = -1;
      if ( CMD_SPACE_SERVICE_SHARD == getFromService() )
      {
         rtnContextDelCL *delContext = NULL;
         rc = rtnCB->contextNew( RTN_CONTEXT_DELCL, (rtnContext **)&delContext,
                                 *pContextID, cb );
         PD_RC_CHECK( rc, PDERROR, "Failed to create context, drop "
                      "collection failed(rc=%d)", rc );
         rc = delContext->open( _collectionName, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open context, drop "
                      "collection failed(rc=%d)",
                      rc );
      }
      else
      {
         rc = rtnDropCollectionCommand ( _collectionName, cb, dmsCB, dpsCB ) ;
         /// AUDIT
         PD_AUDIT_COMMAND( AUDIT_DDL, name(), AUDIT_OBJ_CL,
                           _collectionName, rc, "" ) ;
      }
   done :
      PD_TRACE_EXITRC ( SDB__RTNDROPCL_DOIT, rc ) ;
      return rc ;
   error:
      if ( -1 != *pContextID )
      {
         rtnCB->contextDelete( *pContextID, cb );
         *pContextID = -1;
      }
      goto done;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnDropCollectionspace)

   _rtnDropCollectionspace::_rtnDropCollectionspace ()
   :_spaceName ( NULL )
   {
   }

   _rtnDropCollectionspace::~_rtnDropCollectionspace ()
   {
   }

   const CHAR *_rtnDropCollectionspace::spaceName ()
   {
      return _spaceName ;
   }

   const CHAR *_rtnDropCollectionspace::name ()
   {
      return NAME_DROP_COLLECTIONSPACE ;
   }

   RTN_COMMAND_TYPE _rtnDropCollectionspace::type ()
   {
      return CMD_DROP_COLLECTIONSPACE ;
   }

   BOOLEAN _rtnDropCollectionspace::writable ()
   {
      return TRUE ;
   }

   INT32 _rtnDropCollectionspace::init ( INT32 flags, INT64 numToSkip,
                                         INT64 numToReturn,
                                         const CHAR * pMatcherBuff,
                                         const CHAR * pSelectBuff,
                                         const CHAR * pOrderByBuff,
                                         const CHAR * pHintBuff )
   {
      BSONObj arg ( pMatcherBuff ) ;

      return rtnGetStringElement ( arg, FIELD_NAME_NAME, &_spaceName ) ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDROPCS_DOIT, "_rtnDropCollectionspace::doit" )
   INT32 _rtnDropCollectionspace::doit ( _pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                                         SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                                         INT16 w , INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNDROPCS_DOIT ) ;
      SDB_ASSERT( cb, "cb can't be null!");
      SDB_ASSERT( rtnCB, "rtnCB can't be null!");
      SDB_ASSERT( pContextID, "pContextID can't be null!");
      *pContextID = -1;
      if ( CMD_SPACE_SERVICE_SHARD == getFromService() )
      {
         rtnContextDelCS *delContext = NULL;
         rc = rtnCB->contextNew( RTN_CONTEXT_DELCS,
                                 (rtnContext **)&delContext,
                                 *pContextID, cb );
         PD_RC_CHECK( rc, PDERROR, "Failed to create context, "
                      "drop cs failed(rc=%d)", rc );
         rc = delContext->open( _spaceName, cb );
         PD_RC_CHECK( rc, PDERROR, "Failed to open context, drop cs "
                      "failed(rc=%d)", rc );
      }
      else
      {
         rc = rtnDropCollectionSpaceCommand ( _spaceName, cb, dmsCB, dpsCB ) ;
         /// AUDIT
         PD_AUDIT_COMMAND( AUDIT_DDL, name(), AUDIT_OBJ_CS,
                           _spaceName, rc, "" ) ;
      }
   done :
      PD_TRACE_EXITRC ( SDB__RTNDROPCS_DOIT, rc ) ;
      return rc ;
   error :
      if ( -1 != *pContextID )
      {
         rtnCB->contextDelete( *pContextID, cb );
         *pContextID = -1;
      }
      goto done;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnDropIndex)

   _rtnDropIndex::_rtnDropIndex ()
   :_collectionName ( NULL )
   {
   }

   _rtnDropIndex::~_rtnDropIndex ()
   {
   }

   const CHAR *_rtnDropIndex::name ()
   {
      return NAME_DROP_INDEX ;
   }

   RTN_COMMAND_TYPE _rtnDropIndex::type ()
   {
      return CMD_DROP_INDEX ;
   }

   const CHAR *_rtnDropIndex::collectionFullName ()
   {
      return _collectionName ;
   }

   BOOLEAN _rtnDropIndex::writable ()
   {
      return TRUE ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDROPINDEX_INIT, "_rtnDropIndex::init" )
   INT32 _rtnDropIndex::init ( INT32 flags, INT64 numToSkip,
                               INT64 numToReturn,
                               const CHAR * pMatcherBuff,
                               const CHAR * pSelectBuff,
                               const CHAR * pOrderByBuff,
                               const CHAR * pHintBuff )
   {
      PD_TRACE_ENTRY ( SDB__RTNDROPINDEX_INIT ) ;
      BSONObj arg ( pMatcherBuff ) ;
      INT32 rc = rtnGetStringElement ( arg, FIELD_NAME_COLLECTION,
                                       &_collectionName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to get string[collection]" ) ;
         goto error ;
      }

      rc = rtnGetObjElement ( arg, FIELD_NAME_INDEX, _index ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to get index object " ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC ( SDB__RTNDROPINDEX_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDROPINDEX_DOIT, "_rtnDropIndex::doit" )
   INT32 _rtnDropIndex::doit ( _pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                               SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                               INT16 w , INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNDROPINDEX_DOIT ) ;
      BOOLEAN isSys = FALSE ;
      BSONElement ele = _index.firstElement() ;

      if ( !pmdGetOptionCB()->authEnabled() )
      {
         isSys = TRUE ;
      }
      rc = rtnDropIndexCommand ( _collectionName, ele, cb, dmsCB,
                                 dpsCB, isSys ) ;
      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         /// AUDIT
         PD_AUDIT_COMMAND( AUDIT_DDL, name(), AUDIT_OBJ_CL,
                           _collectionName, rc, "IndexDef:%s",
                           _index.toString().c_str() ) ;
      }
      PD_TRACE_EXITRC ( SDB__RTNDROPINDEX_DOIT, rc ) ;
      return rc ;
   }

   _rtnGet::_rtnGet ()
      :_collectionName ( NULL ), _matcherBuff ( NULL ), _selectBuff ( NULL ),
      _orderByBuff ( NULL )
   {
      _flags = 0 ;
      _numToReturn = -1 ;
      _numToSkip = 0 ;
      _hintExist = FALSE ;
   }

   _rtnGet::~_rtnGet ()
   {
   }

   const CHAR *_rtnGet::collectionFullName ()
   {
      return _collectionName ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNGET_INIT, "_rtnGet::init" )
   INT32 _rtnGet::init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                         const CHAR * pMatcherBuff, const CHAR * pSelectBuff,
                         const CHAR * pOrderByBuff, const CHAR * pHintBuff )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNGET_INIT ) ;
      _flags = flags ;
      _numToReturn = numToReturn ;
      _numToSkip = numToSkip ;
      _matcherBuff = pMatcherBuff ;
      _selectBuff = pSelectBuff ;
      _orderByBuff = pOrderByBuff ;

      BSONObj object ( pHintBuff ) ;
      rc = rtnGetStringElement ( object, FIELD_NAME_COLLECTION,
                                 &_collectionName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s], rc: %d",
                   FIELD_NAME_COLLECTION, rc ) ;

      rc = rtnGetObjElement( object, FIELD_NAME_HINT, _hintObj ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_OK ;
      }
      else
      {
         _hintExist = TRUE ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s], rc: %d",
                   FIELD_NAME_HINT, rc ) ;

   done:
      PD_TRACE_EXITRC ( SDB__RTNGET_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNGET_DOIT, "_rtnGet::doit" )
   INT32 _rtnGet::doit( _pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                        SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                        INT16 w , INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNGET_DOIT ) ;

      SDB_ASSERT ( cb, "educb can't be NULL" ) ;
      SDB_ASSERT ( pContextID, "context id can't be NULL" ) ;

      BSONObj matcher ;
      BSONObj selector ;
      BSONObj orderBy ;

      if ( _matcherBuff )
      {
         matcher = BSONObj( _matcherBuff ) ;
      }
      if ( _selectBuff )
      {
         selector = BSONObj( _selectBuff ) ;
      }
      if ( _orderByBuff )
      {
         orderBy = BSONObj( _orderByBuff ) ;
      }

      rc = rtnGetCommandEntry ( type(), _collectionName, selector, matcher,
                                orderBy, _hintObj, _flags, cb , _numToSkip,
                                _numToReturn, dmsCB, rtnCB, *pContextID ) ;
      PD_TRACE_EXITRC ( SDB__RTNGET_DOIT, rc ) ;
      return rc ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnGetCount)
   _rtnGetCount::_rtnGetCount ()
   {
   }

   _rtnGetCount::~_rtnGetCount ()
   {
   }

   const CHAR *_rtnGetCount::name ()
   {
      return NAME_GET_COUNT ;
   }

   RTN_COMMAND_TYPE _rtnGetCount::type ()
   {
      return CMD_GET_COUNT ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnGetIndexes)
   _rtnGetIndexes::_rtnGetIndexes ()
   {
   }

   _rtnGetIndexes::~_rtnGetIndexes ()
   {
   }

   const CHAR *_rtnGetIndexes::name ()
   {
      return NAME_GET_INDEXES ;
   }

   RTN_COMMAND_TYPE _rtnGetIndexes::type ()
   {
      return CMD_GET_INDEXES ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnGetDatablocks)
   _rtnGetDatablocks::_rtnGetDatablocks ()
   {
   }

   _rtnGetDatablocks::~_rtnGetDatablocks ()
   {
   }

   const CHAR *_rtnGetDatablocks::name ()
   {
      return NAME_GET_DATABLOCKS ;
   }

   RTN_COMMAND_TYPE _rtnGetDatablocks::type ()
   {
      return CMD_GET_DATABLOCKS ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnGetQueryMeta)
   _rtnGetQueryMeta::_rtnGetQueryMeta ()
   {
   }

   _rtnGetQueryMeta::~_rtnGetQueryMeta ()
   {
   }

   const CHAR *_rtnGetQueryMeta::name ()
   {
      return NAME_GET_QUERYMETA ;
   }

   RTN_COMMAND_TYPE _rtnGetQueryMeta::type ()
   {
      return CMD_GET_QUERYMETA ;
   }

   INT32 _rtnGetQueryMeta::doit( pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                                 SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                                 INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      rtnContextDump *context = NULL ;

      SDB_ASSERT( pContextID, "context id can't be NULL" ) ;

      BSONObj matcher ( _matcherBuff ) ;
      BSONObj orderBy ( _orderByBuff ) ;

      if ( !_hintExist )
      {
         /// compatiable with old version. Old version use selector for hint
         _hintObj = BSONObj( _selectBuff ) ;
      }

      rc = rtnCB->contextNew( RTN_CONTEXT_DUMP, (rtnContext**)&context,
                              *pContextID, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Create context failed, rc: %d", rc ) ;

      rc = context->open( BSONObj(), BSONObj(), -1, 0 ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open context[%lld], rc: %d",
                   *pContextID, rc ) ;

      // sample timetamp
      if ( cb->getMonConfigCB()->timestampON )
      {
         context->getMonCB()->recordStartTimestamp() ;
      }

      rc = rtnGetQueryMeta( _collectionName, matcher, orderBy, _hintObj,
                            dmsCB, cb, context ) ;
      PD_RC_CHECK( rc, PDERROR, "Get collection[%s] query meta failed, "
                   "matcher: %s, orderby: %s, hint: %s, rc: %d",
                   _collectionName, matcher.toString().c_str(),
                   orderBy.toString().c_str(),
                   _hintObj.toString().c_str(), rc ) ;

   done:
      return rc ;
   error:
      if ( -1 != *pContextID )
      {
         rtnCB->contextDelete( *pContextID, cb ) ;
         *pContextID = -1 ;
      }
      goto done ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnRenameCollection)
   _rtnRenameCollection::_rtnRenameCollection ()
      :_oldCollectionName ( NULL ), _newCollectionName ( NULL ), _csName( NULL )
   {
   }

   _rtnRenameCollection::~_rtnRenameCollection ()
   {
   }

   const CHAR *_rtnRenameCollection::name ()
   {
      return NAME_RENAME_COLLECTION ;
   }

   RTN_COMMAND_TYPE _rtnRenameCollection::type ()
   {
      return CMD_RENAME_COLLECTION ;
   }

   BOOLEAN _rtnRenameCollection::writable ()
   {
      return TRUE ;
   }

   const CHAR *_rtnRenameCollection::collectionFullName ()
   {
      return _fullCollectionName.c_str() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRENAMECL_INIT, "_rtnRenameCollection::init" )
   INT32 _rtnRenameCollection::init ( INT32 flags, INT64 numToSkip,
                                      INT64 numToReturn,
                                      const CHAR * pMatcherBuff,
                                      const CHAR * pSelectBuff,
                                      const CHAR * pOrderByBuff,
                                      const CHAR * pHintBuff )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNRENAMECL_INIT ) ;

      try
      {
         BSONObj arg ( pMatcherBuff ) ;

         rc = rtnGetStringElement ( arg, FIELD_NAME_COLLECTIONSPACE, &_csName ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Failed to get string[collectionspace]" ) ;
            goto error ;
         }
         rc = rtnGetStringElement ( arg, FIELD_NAME_OLDNAME,
                                    &_oldCollectionName ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Failed to get string[oldname]" ) ;
            goto error ;
         }
         rc = rtnGetStringElement ( arg, FIELD_NAME_NEWNAME,
                                    &_newCollectionName ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Failed to get string[newname]" ) ;
            goto error ;
         }

         _fullCollectionName = _csName ;
         _fullCollectionName += "." ;
         _fullCollectionName += _oldCollectionName ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__RTNRENAMECL_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRENAMECL_DOIT, "_rtnRenameCollection::doit" )
   INT32 _rtnRenameCollection::doit ( _pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                                      SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                                      INT16 w , INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNRENAMECL_DOIT ) ;
      dmsStorageUnit *su = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_CS ;
      BOOLEAN dmsLock = FALSE ;

      rc = dmsCB->writable ( cb ) ;
      if ( rc )
      {
         goto error ;
      }
      dmsLock = TRUE ;

      rc = rtnCollectionSpaceLock ( _csName, dmsCB, FALSE, &su, suID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to get collection space:%s", _csName ) ;
         goto error ;
      }

      rc = su->data()->renameCollection ( _oldCollectionName,
                                          _newCollectionName,
                                          cb, dpsCB ) ;
      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         /// AUDIT
         PD_AUDIT_COMMAND( AUDIT_DDL, name(), AUDIT_OBJ_CL,
                           _oldCollectionName, rc,
                           "NewCollectionName:%s",
                           _newCollectionName ) ;
      }

      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to rename collection from %s to %s",
            _oldCollectionName, _newCollectionName ) ;
         goto error ;
      }

      /// clean apm
      su->getAPM()->invalidatePlans( _oldCollectionName ) ;

   done:
      if ( suID != DMS_INVALID_CS )
      {
         dmsCB->suUnlock ( suID ) ;
      }
      if ( dmsLock )
      {
         dmsCB->writeDown ( cb ) ;
      }
      PD_TRACE_EXITRC ( SDB__RTNRENAMECL_DOIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnRenameCollectionSpace)
   _rtnRenameCollectionSpace::_rtnRenameCollectionSpace()
   {
      _oldName = NULL ;
      _newName = NULL ;
   }
   _rtnRenameCollectionSpace::~_rtnRenameCollectionSpace()
   {
   }

   INT32 _rtnRenameCollectionSpace::init ( INT32 flags, INT64 numToSkip,
                                           INT64 numToReturn,
                                           const CHAR * pMatcherBuff,
                                           const CHAR * pSelectBuff,
                                           const CHAR * pOrderByBuff,
                                           const CHAR * pHintBuff )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObj arg ( pMatcherBuff ) ;
         rc = rtnGetStringElement ( arg, FIELD_NAME_OLDNAME,
                                    &_oldName ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Failed to get string[oldname]" ) ;
            goto error ;
         }
         rc = rtnGetStringElement ( arg, FIELD_NAME_NEWNAME,
                                    &_newName ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Failed to get string[newname]" ) ;
            goto error ;
         }

         rc = dmsCheckCSName( _oldName, FALSE ) ;
         if ( rc )
         {
            goto error ;
         }
         rc = dmsCheckCSName( _newName, FALSE ) ;
         if ( rc )
         {
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRENAMECS_DOIT, "_rtnRenameCollectionSpace::doit" )
   INT32 _rtnRenameCollectionSpace::doit ( _pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                                           SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                                           INT16 w , INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNRENAMECS_DOIT ) ;
      BOOLEAN dmsLock = FALSE ;

      rc = dmsCB->writable ( cb ) ;
      if ( rc )
      {
         goto error ;
      }
      dmsLock = TRUE ;

      // let's find out whether the collection space is held by this
      // EDU. If so we have to get rid of those contexts
      if ( NULL != cb )
      {
         rtnDelContextForCollectionSpace( _oldName, cb ) ;
      }

      rc = dmsCB->renameCollectionSpace( _oldName, _newName, cb, dpsCB ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to rename collectionspace from %s to %s",
                  _oldName, _newName ) ;
         goto error ;
      }
      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         /// AUDIT
         PD_AUDIT_COMMAND( AUDIT_DDL, name(), AUDIT_OBJ_CS,
                           _oldName, rc,
                           "NewCollectionSpaceName:%s",
                           _newName ) ;
      }

   done:
      if ( dmsLock )
      {
         dmsCB->writeDown( cb ) ;
      }
      PD_TRACE_EXITRC ( SDB__RTNRENAMECS_DOIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   _rtnReorg::_rtnReorg ()
      :_collectionName ( NULL ), _hintBuffer ( NULL )
   {
   }

   _rtnReorg::~_rtnReorg ()
   {
   }

   INT32 _rtnReorg::spaceNode()
   {
      return CMD_SPACE_NODE_STANDALONE ;
   }

   const CHAR *_rtnReorg::collectionFullName ()
   {
      return _collectionName ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNREORG_INIT, "_rtnReorg::init" )
   INT32 _rtnReorg::init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                           const CHAR * pMatcherBuff, const CHAR * pSelectBuff,
                           const CHAR * pOrderByBuff, const CHAR * pHintBuff )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNREORG_INIT ) ;

      _hintBuffer = pHintBuff ;
      BSONObj arg ( pMatcherBuff ) ;
      rc = rtnGetStringElement ( arg, FIELD_NAME_COLLECTION,
                                 &_collectionName ) ;
      PD_TRACE_EXITRC ( SDB__RTNREORG_INIT, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNREORG_DOIT, "_rtnReorg::doit" )
   INT32 _rtnReorg::doit ( _pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                           SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                           INT16 w , INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNREORG_DOIT ) ;
      BSONObj hint ( _hintBuffer ) ;

      rc = dmsCB->writable ( cb ) ;
      if ( rc )
      {
         goto done ;
      }

      switch ( type() )
      {
         case CMD_REORG_OFFLINE :
            rc = rtnReorgOffline ( _collectionName, hint, cb, dmsCB, rtnCB ) ;
            break ;
         case CMD_REORG_ONLINE :
            PD_LOG ( PDERROR, "Online reorg is not supported yet" ) ;
            rc = SDB_SYS ;
            break ;
         case CMD_REORG_RECOVER :
            rc = rtnReorgRecover ( _collectionName, cb, dmsCB, rtnCB ) ;
            break ;
         default :
            PD_LOG ( PDEVENT, "Unknow reorg command" ) ;
            rc = SDB_INVALIDARG ;
            break ;
      }
      dmsCB->writeDown ( cb ) ;

      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         /// AUDIT
         PD_AUDIT_COMMAND( AUDIT_DDL, name(), AUDIT_OBJ_CL,
                           _collectionName, rc, "" ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__RTNREORG_DOIT, rc ) ;
      return rc ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnReorgOffline)
   _rtnReorgOffline::_rtnReorgOffline ()
   {
   }

   _rtnReorgOffline::~_rtnReorgOffline ()
   {
   }

   const CHAR *_rtnReorgOffline::name ()
   {
      return NAME_REORG_OFFLINE ;
   }

   RTN_COMMAND_TYPE _rtnReorgOffline::type ()
   {
      return CMD_REORG_OFFLINE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnReorgOnline)
   _rtnReorgOnline::_rtnReorgOnline ()
   {
   }

   _rtnReorgOnline::~_rtnReorgOnline ()
   {
   }

   const CHAR *_rtnReorgOnline::name ()
   {
      return NAME_REORG_ONLINE ;
   }

   RTN_COMMAND_TYPE _rtnReorgOnline::type ()
   {
      return CMD_REORG_ONLINE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnReorgRecover)
   _rtnReorgRecover::_rtnReorgRecover ()
   {
   }

   _rtnReorgRecover::~_rtnReorgRecover ()
   {
   }

   const CHAR *_rtnReorgRecover::name ()
   {
      return NAME_REORG_RECOVER ;
   }

   RTN_COMMAND_TYPE _rtnReorgRecover::type ()
   {
      return CMD_REORG_RECOVER ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnShutdown)
   _rtnShutdown::_rtnShutdown ()
   {
   }

   _rtnShutdown::~_rtnShutdown ()
   {
   }

   const CHAR *_rtnShutdown::name ()
   {
      return NAME_SHUTDOWN ;
   }

   RTN_COMMAND_TYPE _rtnShutdown::type ()
   {
      return CMD_SHUTDOWN ;
   }

   INT32 _rtnShutdown::init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR * pMatcherBuff,
                              const CHAR * pSelectBuff,
                              const CHAR * pOrderByBuff,
                              const CHAR * pHintBuff )
   {
      return SDB_OK ;
   }

   INT32 _rtnShutdown::doit( _pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                             SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                             INT16 w , INT64 *pContextID )
   {
      PD_LOG( PDEVENT, "Shut down sevice" ) ;
      PMD_SHUTDOWN_DB( SDB_OK ) ;

      /// AUDIT
      PD_AUDIT_COMMAND( AUDIT_SYSTEM, name(), AUDIT_OBJ_NODE,
                        "", SDB_OK, "" ) ;

      return SDB_OK ;
   }

   _rtnTest::_rtnTest ()
      :_collectionName ( NULL )
   {
   }

   _rtnTest::~_rtnTest ()
   {
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNTEST_INIT, "_rtnTest::init" )
   INT32 _rtnTest::init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                          const CHAR * pMatcherBuff, const CHAR * pSelectBuff,
                          const CHAR * pOrderByBuff, const CHAR * pHintBuff )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNTEST_INIT ) ;
      BSONObj arg ( pMatcherBuff ) ;
      rc = rtnGetStringElement ( arg, FIELD_NAME_NAME, &_collectionName ) ;
      PD_TRACE_EXITRC ( SDB__RTNTEST_INIT, rc ) ;
      return rc ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNTEST_DOIT, "_rtnTest::doit" )
   INT32 _rtnTest::doit ( _pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                          SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                          INT16 w , INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNTEST_DOIT ) ;

      switch ( type () )
      {
         case CMD_TEST_COLLECTION :
            rc = rtnTestCollectionCommand ( _collectionName , dmsCB ) ;
            break ;
         case CMD_TEST_COLLECTIONSPACE :
            rc = rtnTestCollectionSpaceCommand ( _collectionName, dmsCB ) ;
            break ;
         default :
            PD_LOG ( PDEVENT, "Unknow test command " ) ;
            rc = SDB_INVALIDARG ;
            break ;
      }

      PD_TRACE_EXITRC ( SDB__RTNTEST_DOIT, rc ) ;
      return rc ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnTestCollection)
   _rtnTestCollection::_rtnTestCollection ()
   {
   }

   _rtnTestCollection::~_rtnTestCollection ()
   {
   }

   const CHAR *_rtnTestCollection::name ()
   {
      return NAME_TEST_COLLECTION ;
   }

   RTN_COMMAND_TYPE _rtnTestCollection::type ()
   {
      return CMD_TEST_COLLECTION ;
   }

   const CHAR *_rtnTestCollection::collectionFullName ()
   {
      return _collectionName ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnTestCollectionspace)
   _rtnTestCollectionspace::_rtnTestCollectionspace ()
   {
   }

   _rtnTestCollectionspace::~_rtnTestCollectionspace ()
   {
   }

   const CHAR *_rtnTestCollectionspace::name ()
   {
      return NAME_TEST_COLLECTIONSPACE ;
   }

   RTN_COMMAND_TYPE _rtnTestCollectionspace::type ()
   {
      return CMD_TEST_COLLECTIONSPACE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSetPDLevel)
   _rtnSetPDLevel::_rtnSetPDLevel()
   {
      _pdLevel = PDEVENT ;
   }

   _rtnSetPDLevel::~_rtnSetPDLevel()
   {
   }

   const CHAR* _rtnSetPDLevel::name ()
   {
      return NAME_SET_PDLEVEL ;
   }

   RTN_COMMAND_TYPE _rtnSetPDLevel::type ()
   {
      return CMD_SET_PDLEVEL ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSETPDLEVEL_INIT, "_rtnSetPDLevel::init" )
   INT32 _rtnSetPDLevel::init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                                const CHAR * pMatcherBuff,
                                const CHAR * pSelectBuff,
                                const CHAR * pOrderByBuff,
                                const CHAR * pHintBuff )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNSETPDLEVEL_INIT ) ;
      BSONObj match ( pMatcherBuff ) ;

      rc = rtnGetIntElement( match, FIELD_NAME_PDLEVEL, _pdLevel ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to extract %s field for set pdleve",
                  FIELD_NAME_PDLEVEL ) ;
         goto error ;
      }
      if ( _pdLevel < PDSEVERE || _pdLevel > PDDEBUG )
      {
         PD_LOG ( PDWARNING, "PDLevel[%d] error, set to default[%d]",
                  _pdLevel, PDWARNING ) ;
         _pdLevel = PDWARNING ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__RTNSETPDLEVEL_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSETPDLEVEL_DOIT, "_rtnSetPDLevel::doit" )
   INT32 _rtnSetPDLevel::doit ( _pmdEDUCB * cb, _SDB_DMSCB * dmsCB,
                                _SDB_RTNCB * rtnCB, _dpsLogWrapper * dpsCB,
                                INT16 w, INT64 * pContextID )
   {
      PD_TRACE_ENTRY ( SDB__RTNSETPDLEVEL_DOIT ) ;
      setPDLevel( (PDLEVEL)_pdLevel ) ;
      PD_LOG ( getPDLevel(), "Set PDLEVEL to [%s]",
               getPDLevelDesp( getPDLevel() ) ) ;
      PD_TRACE_EXIT ( SDB__RTNSETPDLEVEL_DOIT ) ;
      return SDB_OK ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnReloadConfig)

   _rtnReloadConfig::_rtnReloadConfig()
   {
   }

   _rtnReloadConfig::~_rtnReloadConfig()
   {
   }

   const CHAR* _rtnReloadConfig::name()
   {
      return NAME_RELOAD_CONFIG ;
   }

   RTN_COMMAND_TYPE _rtnReloadConfig::type()
   {
      return CMD_RELOAD_CONFIG ;
   }

   INT32 _rtnReloadConfig::init( INT32 flags, INT64 numToSkip,
                                 INT64 numToReturn,
                                 const CHAR *pMatcherBuff,
                                 const CHAR * pSelectBuff,
                                 const CHAR * pOrderByBuff,
                                 const CHAR * pHintBuff )
   {
      return SDB_OK ;
   }

   INT32 _rtnReloadConfig::doit( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                 _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                                 INT16 w, INT64 * pContextID )
   {
      INT32 rc = SDB_OK ;

      pmdOptionsCB *optCB = pmdGetOptionCB() ;
      pmdOptionsCB tmpCB ;
      BSONObj cfgObj ;

      rc = tmpCB.initFromFile( optCB->getConfFile(), FALSE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init config file[%s] failed, rc: %d",
                 optCB->getConfFile(), rc ) ;
         goto error ;
      }

      rc = tmpCB.toBSON( cfgObj, PMD_CFG_MASK_SKIP_UNFIELD ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Convert config to bson failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = optCB->change( cfgObj, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Reload config[%s] failed, rc: %d",
                 cfgObj.toString().c_str(), rc ) ;
         goto error ;
      }
      /// dump memory config
      optCB->toBSON( cfgObj ) ;

      PD_LOG( PDEVENT, "Reload config succeed. All configs: %s",
              cfgObj.toString().c_str() ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnTraceStart)
   _rtnTraceStart::_rtnTraceStart ()
   {
      _mask = 0xFFFFFFFF ;
      _size = TRACE_DFT_BUFFER_SIZE ;
   }
   _rtnTraceStart::~_rtnTraceStart ()
   {
   }

   const CHAR *_rtnTraceStart::name ()
   {
      return CMD_NAME_TRACE_START ;
   }

   RTN_COMMAND_TYPE _rtnTraceStart::type ()
   {
      return CMD_TRACE_START ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNTRACESTART_INIT, "_rtnTraceStart::init" )
   INT32 _rtnTraceStart::init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                                const CHAR * pMatcherBuff, const CHAR * pSelectBuff,
                                const CHAR * pOrderByBuff, const CHAR * pHintBuff )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNTRACESTART_INIT ) ;
      try
      {
         BSONObj arg ( pMatcherBuff );
         BSONElement eleTID  = arg.getField ( FIELD_NAME_THREADS ) ;
         BSONElement eleSize = arg.getField ( FIELD_NAME_SIZE ) ;
         UINT32 one = 1 ;
         BSONElement eleComp = arg.getField( FIELD_NAME_COMPONENTS );
         BSONElement eleBreakPoint = arg.getField(FIELD_NAME_BREAKPOINTS);
         if ( eleComp.type() == Array )
         {
            _mask = 0 ;
            BSONObjIterator it ( eleComp.embeddedObject() ) ;
            if ( !it.more () )
            {
               // if there's no element, that means we need mask everything
               for( INT32 i = 0; i < _pdTraceComponentNum; ++i )
               {
                  _mask |= one << i ;
               }
            }
            else
            {
               while ( it.more() )
               {
                  BSONElement ele = it.next() ;
                  if ( ele.type() == String )
                  {
                     for ( INT32 i = 0; i < _pdTraceComponentNum; ++i)
                     {
                        if ( ossStrcmp ( pdGetTraceComponent(i),
                                     ele.valuestr() ) == 0 )
                        {
                           _mask |= one<<i;
                        }
                     } // for
                  } // if ( ele.type() == String )
               } // while ( it.more() )
            }
         } // if ( eleComp.type() == Array )
#if defined (SDB_ENGINE)
         if( eleBreakPoint.type() == Array )
         {
            BSONObjIterator it( eleBreakPoint.embeddedObject() );
            while( it.more() )
            {
               BSONElement ele = it.next();
               if( ele.type() == String )
               {
                  const char * eleStr = ele.valuestr();
                  for( UINT64 i = 0; i < pdGetTraceFunctionListNum(); i++ )
                  {
                     const CHAR *  funcName = pdGetTraceFunction( i );
                     if( 0 == ossStrcmp( funcName, eleStr ) )
                     {
                        _funcCode.push_back ( i ) ;
                        // do NOT break since we may have functions with
                        // duplicate names
                     } // if( 0 == ossStrcmp( funcName, eleStr ) )
                  } // for( UINT64 i = 0; i < pdGetTraceFunctionListNum(); i++ )
               } // if( ele.type() == String )
            } // while( it.more() )
         }
#endif
         if ( eleTID.type() == Array )
         {
            BSONObjIterator it ( eleTID.embeddedObject() ) ;
            while ( it.more () )
            {
               BSONElement ele = it.next() ;
               if ( ele.isNumber() )
               {
                  _tid.push_back ( (UINT32)ele.numberLong() ) ;
               }
            } // while ( it.more () )
         } // if ( eleTID.type() == Array )
         if ( eleSize.isNumber() )
         {
            _size = (UINT32)eleSize.numberLong() ;
         }
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK ( SDB_SYS, PDERROR,
                       "Exception when initialize trace start command: %s",
                       e.what() ) ;
      }
   done :
      PD_TRACE_EXITRC ( SDB__RTNTRACESTART_INIT, rc ) ;
      return rc ;
   error :
      goto done ;
   }



   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNTRACESTART_DOIT, "_rtnTraceStart::doit" )
   INT32 _rtnTraceStart::doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                                INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNTRACESTART_DOIT ) ;
      rc = sdbGetPDTraceCB()->start ( (UINT64)_size, _mask, &_funcCode ) ;
      PD_TRACE_EXITRC ( SDB__RTNTRACESTART_DOIT, rc ) ;
      return rc ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnTraceResume)
   _rtnTraceResume::_rtnTraceResume ()
   {
   }
   _rtnTraceResume::~_rtnTraceResume ()
   {
   }

   const CHAR *_rtnTraceResume::name ()
   {
      return CMD_NAME_TRACE_RESUME ;
   }

   RTN_COMMAND_TYPE _rtnTraceResume::type ()
   {
      return CMD_TRACE_RESUME ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNTRACERESUME_INIT, "_rtnTraceResume::init" )
   INT32 _rtnTraceResume::init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                                const CHAR * pMatcherBuff, const CHAR * pSelectBuff,
                                const CHAR * pOrderByBuff, const CHAR * pHintBuff )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNTRACERESUME_INIT ) ;
      PD_TRACE_EXITRC ( SDB__RTNTRACERESUME_INIT, rc ) ;
      return rc ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNTRACERESUME_DOIT, "_rtnTraceResume::doit" )
   INT32 _rtnTraceResume::doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                                INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNTRACERESUME_DOIT ) ;
      pdTraceCB *pdTraceCB = sdbGetPDTraceCB() ;
      pdTraceCB->removeAllBreakPoint () ;
      // sleep for a second so that break point removal information is broadcast
      // to all CPUs
      // Note this is not performance sensitive code, so it's safe to sleep for
      // 1 second
      ossSleepsecs(1) ;
      pdTraceCB->resumePausedEDUs () ;
      PD_TRACE_EXITRC ( SDB__RTNTRACERESUME_DOIT, rc ) ;
      return rc ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnTraceStop)
   _rtnTraceStop::_rtnTraceStop ()
   {
      _pDumpFileName = FALSE ;
   }
   _rtnTraceStop::~_rtnTraceStop ()
   {
   }

   const CHAR *_rtnTraceStop::name ()
   {
      return CMD_NAME_TRACE_STOP ;
   }

   RTN_COMMAND_TYPE _rtnTraceStop::type ()
   {
      return CMD_TRACE_STOP ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNTRACESTOP_INIT, "_rtnTraceStop::init" )
   INT32 _rtnTraceStop::init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                               const CHAR * pMatcherBuff, const CHAR * pSelectBuff,
                               const CHAR * pOrderByBuff, const CHAR * pHintBuff )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNTRACESTOP_INIT ) ;
      try
      {
         BSONObj arg ( pMatcherBuff ) ;
         BSONElement eleDumpName = arg.getField ( FIELD_NAME_FILENAME ) ;
         if ( eleDumpName.type() == String )
         {
            _pDumpFileName = eleDumpName.valuestr() ;
         }
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK ( SDB_SYS, PDERROR,
                       "Exception when initialize trace stop command: %s",
                       e.what() ) ;
      }
   done :
      PD_TRACE_EXITRC ( SDB__RTNTRACESTOP_INIT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNTRACESTOP_DOIT, "_rtnTraceStop::doit" )
   INT32 _rtnTraceStop::doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                               _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                               INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNTRACESTOP_DOIT ) ;
      pdTraceCB *traceCB = sdbGetPDTraceCB() ;
      if ( _pDumpFileName )
      {
         traceCB->stop() ;
         rc = traceCB->dump ( _pDumpFileName ) ;
         PD_RC_CHECK ( rc, PDWARNING, "Failed to stop trace, rc = %d", rc ) ;
      }
   done :
      traceCB->destroy() ;
      PD_TRACE_EXITRC ( SDB__RTNTRACESTOP_DOIT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnTraceStatus)
   _rtnTraceStatus::_rtnTraceStatus ()
   {
   }
   _rtnTraceStatus::~_rtnTraceStatus ()
   {
   }

   const CHAR *_rtnTraceStatus::name ()
   {
      return CMD_NAME_TRACE_STATUS ;
   }

   RTN_COMMAND_TYPE _rtnTraceStatus::type ()
   {
      return CMD_TRACE_STATUS ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNTRACESTATUS_INIT, "_rtnTraceStatus::init" )
   INT32 _rtnTraceStatus::init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                                 const CHAR * pMatcherBuff, const CHAR * pSelectBuff,
                                 const CHAR * pOrderByBuff, const CHAR * pHintBuff )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNTRACESTATUS_INIT ) ;
      _flags = flags ;
      _numToReturn = numToReturn ;
      _numToSkip = numToSkip ;
      _matcherBuff = pMatcherBuff ;
      _selectBuff = pSelectBuff ;
      _orderByBuff = pOrderByBuff ;

      PD_TRACE_EXITRC ( SDB__RTNTRACESTATUS_INIT, rc ) ;
      return rc ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNTRACESTATUS_DOIT, "_rtnTraceStatus::doit" )
   INT32 _rtnTraceStatus::doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                 _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                                 INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT ( cb, "cb can't be NULL" ) ;
      SDB_ASSERT ( pContextID, "context id can't be NULL" ) ;
      PD_TRACE_ENTRY ( SDB__RTNTRACESTATUS_DOIT ) ;
      rtnContextDump *context = NULL ;
      *pContextID = -1 ;
      // create cursors
      rc = rtnCB->contextNew ( RTN_CONTEXT_DUMP, (rtnContext**)&context,
                               *pContextID, cb ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to create new context, rc = %d", rc ) ;

      if ( cb->getMonConfigCB()->timestampON )
      {
         context->getMonCB()->recordStartTimestamp() ;
      }

      try
      {
         BSONObj matcher ( _matcherBuff ) ;
         BSONObj selector ( _selectBuff ) ;
         BSONObj orderBy ( _orderByBuff ) ;

         rc = context->open( selector,
                             matcher,
                             orderBy.isEmpty() ? _numToReturn : -1,
                             orderBy.isEmpty() ? _numToSkip : 0 ) ;
         PD_RC_CHECK( rc, PDERROR, "Open context failed, rc: %d", rc ) ;

         rc = monDumpTraceStatus ( context ) ;
         PD_RC_CHECK ( rc, PDERROR,
                       "Failed to dump trace status, rc = %d", rc ) ;

         if ( !orderBy.isEmpty() )
         {
            rc = rtnSort( (rtnContext**)&context,
                          orderBy,
                          cb, _numToSkip,
                          _numToReturn, *pContextID ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to sort, rc: %d", rc ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK ( SDB_SYS, PDERROR,
                       "Exception when extracting trace status: %s", e.what() );
      }

   done :
      PD_TRACE_EXITRC ( SDB__RTNTRACESTATUS_DOIT, rc ) ;
      return rc ;
   error :
      if ( context )
      {
         rtnCB->contextDelete ( *pContextID, cb ) ;
         *pContextID = -1 ;
      }
      goto done ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnLoad)
   _rtnLoad::_rtnLoad ()
   {
      _parameters.pFieldArray = NULL ;
   }
   _rtnLoad::~_rtnLoad ()
   {
      SAFE_OSS_FREE ( _parameters.pFieldArray ) ;
   }

   const CHAR *_rtnLoad::name ()
   {
      return CMD_NAME_JSON_LOAD ;
   }

   RTN_COMMAND_TYPE _rtnLoad::type ()
   {
      return CMD_JSON_LOAD ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOAD_INIT, "_rtnLoad::init" )
   INT32 _rtnLoad::init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                          const CHAR * pMatcherBuff, const CHAR * pSelectBuff,
                          const CHAR * pOrderByBuff, const CHAR * pHintBuff )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNLOAD_INIT ) ;
      MIG_PARSER_FILE_TYPE fileType       = MIG_PARSER_JSON ;
      UINT32               threadNum      = MIG_THREAD_NUMBER ;
      UINT32               bucketNum      = MIG_BUCKET_NUMBER ;
      UINT32               bufferSize     = MIG_SUM_BUFFER_SIZE ;
      BOOLEAN              isAsynchronous = FALSE ;
      BOOLEAN              headerline     = FALSE ;
      CHAR                *fields         = NULL ;
      const CHAR          *tempValue      = NULL ;
      CHAR                 delCFR[4] ;
      BSONElement          tempEle ;

      ossMemset ( _fileName, 0, OSS_MAX_PATHSIZE + 1 ) ;
      ossMemset ( _csName,   0, DMS_COLLECTION_SPACE_NAME_SZ + 1 ) ;
      ossMemset ( _clName,   0, DMS_COLLECTION_NAME_SZ + 1 ) ;
      delCFR[0] = '"' ;
      delCFR[1] = ',' ;
      delCFR[2] = '\n' ;
      delCFR[3] = 0 ;

      try
      {
         BSONObj obj( pMatcherBuff ) ;
         //file name
         tempEle = obj.getField ( FIELD_NAME_FILENAME ) ;
         if ( tempEle.eoo() )
         {
            PD_LOG ( PDERROR, "No file name" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( String != tempEle.type() )
         {
            PD_LOG ( PDERROR, "file name is not a string" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         tempValue = tempEle.valuestr() ;
         ossStrncpy ( _fileName, tempValue, tempEle.valuestrsize() ) ;

         //cs name
         tempEle = obj.getField ( FIELD_NAME_COLLECTIONSPACE ) ;
         if ( tempEle.eoo() )
         {
            PD_LOG ( PDERROR, "No collection space name" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( String != tempEle.type() )
         {
            PD_LOG ( PDERROR, "collection space name is not a string" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         tempValue = tempEle.valuestr() ;
         ossStrncpy ( _csName, tempValue, tempEle.valuestrsize() ) ;

         //cl name
         tempEle = obj.getField ( FIELD_NAME_COLLECTION ) ;
         if ( tempEle.eoo() )
         {
            PD_LOG ( PDERROR, "No collection name" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( String != tempEle.type() )
         {
            PD_LOG ( PDERROR, "collection name is not a string" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         tempValue = tempEle.valuestr() ;
         ossStrncpy ( _clName, tempValue, tempEle.valuestrsize() ) ;

         //fields
         tempEle = obj.getField ( FIELD_NAME_FIELDS ) ;
         if ( !tempEle.eoo() )
         {
            INT32 fieldsSize = 0 ;
            if ( String != tempEle.type() )
            {
               PD_LOG ( PDERROR, "fields is not a string" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            tempValue = tempEle.valuestr() ;
            fieldsSize = tempEle.valuestrsize() ;
            fields = (CHAR *)SDB_OSS_MALLOC( fieldsSize ) ;
            if ( !fields )
            {
               PD_LOG ( PDERROR,
                        "Unable to allocate %d bytes memory", fieldsSize ) ;
               rc = SDB_OOM ;
               goto error ;
            }
            ossStrncpy ( fields,
                         tempValue, fieldsSize ) ;
         }

         //character
         tempEle = obj.getField ( FIELD_NAME_CHARACTER ) ;
         if ( !tempEle.eoo() )
         {
            INT32 fieldsSize = 0 ;
            if ( String != tempEle.type() )
            {
               PD_LOG ( PDERROR, "fields is not a string" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            tempValue = tempEle.valuestr() ;
            fieldsSize = tempEle.valuestrsize() ;
            if ( fieldsSize == 4 )
            {
               ossStrncpy ( delCFR, tempValue, fieldsSize ) ;
               delCFR[3] = 0 ;
            }
            else
            {
               PD_LOG ( PDERROR, "error character" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }

         // asynchronous
         tempEle = obj.getField ( FIELD_NAME_ASYNCHRONOUS ) ;
         if ( !tempEle.eoo() )
         {
            if ( Bool != tempEle.type() )
            {
               PD_LOG ( PDERROR, "asynchronous is not a bool" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            isAsynchronous = tempEle.boolean() ;
         }

         // headerline
         tempEle = obj.getField ( FIELD_NAME_HEADERLINE ) ;
         if ( !tempEle.eoo() )
         {
            if ( Bool != tempEle.type() )
            {
               PD_LOG ( PDERROR, "headerline is not a bool" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            headerline = tempEle.boolean() ;
         }

         // thread number
         tempEle = obj.getField ( FIELD_NAME_THREADNUM ) ;
         if ( !tempEle.eoo() )
         {
            if ( NumberInt != tempEle.type() )
            {
               PD_LOG ( PDERROR, "thread number is not a number" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            threadNum = (UINT32)tempEle.Int() ;
         }

         // bucket number
         tempEle = obj.getField ( FIELD_NAME_BUCKETNUM ) ;
         if ( !tempEle.eoo() )
         {
            if ( NumberInt != tempEle.type() )
            {
               PD_LOG ( PDERROR, "bucket number is not a number" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            bucketNum = (UINT32)tempEle.Int() ;
         }

         // buffer size
         tempEle = obj.getField ( FIELD_NAME_PARSEBUFFERSIZE ) ;
         if ( !tempEle.eoo() )
         {
            if ( NumberInt != tempEle.type() )
            {
               PD_LOG ( PDERROR, "buffer size is not a number" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            bufferSize = (UINT32)tempEle.Int() ;
         }

         // type
         tempEle = obj.getField ( FIELD_NAME_LTYPE ) ;
         if ( tempEle.eoo() )
         {
            PD_LOG ( PDERROR, "file type must input" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( NumberInt != tempEle.type() )
         {
            PD_LOG ( PDERROR, "file type is not a number" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         fileType = (MIG_PARSER_FILE_TYPE)tempEle.Int() ;

      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Failed to create BSON object: %s",
                  e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      _parameters.pFieldArray          = fields ;
      _parameters.headerline           = headerline ;
      _parameters.fileType             = fileType ;
      _parameters.pFileName            = _fileName ;
      _parameters.pCollectionSpaceName = _csName ;
      _parameters.pCollectionName      = _clName ;
      _parameters.bufferSize           = bufferSize ;
      _parameters.bucketNum            = bucketNum ;
      _parameters.workerNum            = threadNum ;
      _parameters.isAsynchronous       = isAsynchronous ;
      _parameters.port                 = NULL ;
      _parameters.delCFR[0]            = delCFR[0] ;
      _parameters.delCFR[1]            = delCFR[1] ;
      _parameters.delCFR[2]            = delCFR[2] ;
      _parameters.delCFR[3]            = delCFR[3] ;
   done:
      PD_TRACE_EXITRC ( SDB__RTNLOAD_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOAD_DOIT, "_rtnLoad::doit" )
   INT32 _rtnLoad::doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                          _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                          INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT ( cb, "cb can't be NULL" ) ;
      SDB_ASSERT ( pContextID, "context id can't be NULL" ) ;
      PD_TRACE_ENTRY ( SDB__RTNLOAD_DOIT ) ;
      migMaster master ;
      _parameters.clientSock = cb->getClientSock() ;

      rc = master.initialize( &_parameters ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to init migMaster, rc=%d", rc ) ;
         goto error ;
      }

      rc = master.run( cb ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to run migMaster, rc=%d", rc ) ;
         goto error ;
      }
   done :
      PD_TRACE_EXITRC ( SDB__RTNLOAD_DOIT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnExportConf)
   // PD_TRACE_DECLARE_FUNCTION( SDB__RTNEXPCONF, "_rtnExportConf::doit" )
   INT32 _rtnExportConf::doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNEXPCONF ) ;
      rc = pmdGetKRCB()->getOptionCB()->reflush2File() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to export configration:%d",rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__RTNEXPCONF, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnRemoveBackup)
   _rtnRemoveBackup::_rtnRemoveBackup()
   {
      _path          = NULL ;
      _backupName    = NULL ;
      _matcherBuff   = NULL ;
   }

   INT32 _rtnRemoveBackup::init( INT32 flags, INT64 numToSkip,
                                 INT64 numToReturn,
                                 const CHAR * pMatcherBuff,
                                 const CHAR * pSelectBuff,
                                 const CHAR * pOrderByBuff,
                                 const CHAR * pHintBuff )
   {
      INT32 rc = SDB_OK ;
      _matcherBuff = pMatcherBuff ;
      try
      {
         BSONObj matcher( _matcherBuff ) ;
         rc = rtnGetStringElement( matcher, FIELD_NAME_PATH, &_path ) ;
         if ( rc == SDB_FIELD_NOT_EXIST )
         {
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDWARNING, "Failed to get field[%s], rc: %d",
                      FIELD_NAME_PATH, rc ) ;
         rc = rtnGetStringElement( matcher, FIELD_NAME_NAME, &_backupName ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDWARNING, "Failed to get field[%s], rc: %d",
                      FIELD_NAME_NAME, rc ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnRemoveBackup::doit( pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                                 SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                                 INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      try
      {
         BSONObj matcher( _matcherBuff ) ;
         rc = rtnRemoveBackup( cb, _path, _backupName, matcher ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
      }
      return rc ;
   }

   _rtnForceSession::_rtnForceSession()
   :_sessionID( OSS_INVALID_PID )
   {

   }

   _rtnForceSession::~_rtnForceSession()
   {

   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnForceSession )
   INT32 _rtnForceSession::init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                                  const CHAR *pMatcherBuff,
                                  const CHAR *pSelectBuff,
                                  const CHAR *pOrderByBuff,
                                  const CHAR *pHintBuff )
   {
      INT32 rc = SDB_OK ;
      try
      {
         BSONObj obj( pMatcherBuff ) ;
         BSONElement sessionID = obj.getField( FIELD_NAME_SESSIONID ) ;
         if ( !sessionID.isNumber() )
         {
            PD_LOG( PDERROR, "session id should be a number:%s",
                    obj.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         _sessionID = sessionID.numberLong() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnForceSession::doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                  _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                                  INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      pmdEDUMgr *mgr = pmdGetKRCB()->getEDUMgr() ;

      if ( _sessionID == cb->getID() )
      {
         PD_LOG( PDDEBUG, "Force self[%lld], ignored", _sessionID ) ;
         goto done ;
      }

      rc = mgr->forceUserEDU( _sessionID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to force edu:%lld, rc:%d",
                 _sessionID, rc ) ;
         goto error ;
      }
      else
      {
         PD_LOG( PDEVENT, "Force session[%lld] succeed", _sessionID ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListLob)
   _rtnListLob::_rtnListLob()
   :_contextID( -1 ),
    _fullName( NULL )
   {

   }

   _rtnListLob::~_rtnListLob()
   {

   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLISTLOB_INIT, "_rtnListLob::init" )
   INT32 _rtnListLob::init( INT32 flags, INT64 numToSkip,
                            INT64 numToReturn,
                            const CHAR *pMatcherBuff,
                            const CHAR *pSelectBuff,
                            const CHAR *pOrderByBuff,
                            const CHAR *pHintBuff )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNLISTLOB_INIT ) ;
      try
      {
         _query = BSONObj( pMatcherBuff ) ;
         BSONElement ele = _query.getField( FIELD_NAME_COLLECTION ) ;
         if ( String != ele.type() )
         {
            PD_LOG( PDERROR, "invalid collection name in condition:%s",
                    _query.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         _fullName = ele.valuestr() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNLISTLOB_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   const CHAR *_rtnListLob::collectionFullName()
   {
      return _fullName ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLISTLOB_DOIT, "_rtnListLob::doit" )
   INT32 _rtnListLob::doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                             _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                             INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNLISTLOB_DOIT ) ;
      rtnContextListLob *context = NULL ;

      rc = rtnCB->contextNew( RTN_CONTEXT_LIST_LOB,
                              (rtnContext**)(&context),
                              _contextID, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open lob context:%d", rc ) ;
         goto error ;
      }

      SDB_ASSERT( NULL != context, "can not be null" ) ;
      rc = context->open( _query, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open list lob context:%d", rc ) ;
         goto error ;
      }

      *pContextID = _contextID ;
   done:
      PD_TRACE_EXITRC( SDB__RTNLISTLOB_DOIT, rc ) ;
      return rc ;
   error:
      if ( -1 != _contextID )
      {
         rtnCB->contextDelete ( _contextID, cb ) ;
         _contextID = -1 ;
      }
      goto done ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnSetSessionAttr )

   // PD_TRACE_DECLARE_FUNCTION( SDB__RTNSETSESSATTR_INIT, "_rtnSetSessionAttr::init" )
   INT32 _rtnSetSessionAttr::init( INT32 flags, INT64 numToSkip,
                                   INT64 numToReturn,
                                   const CHAR *pMatcherBuff,
                                   const CHAR *pSelectBuff,
                                   const CHAR *pOrderByBuff,
                                   const CHAR *pHintBuff )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNSETSESSATTR_INIT ) ;

      try
      {
         BSONObj property( pMatcherBuff ) ;
         rtnSessionProperty sessProperty ;

         rc = sessProperty.parseProperty( property ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse session property, "
                      "rc: %d", rc ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Failed to set sessionAttr, received unexpected "
                 "error:%s", e.what() ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNSETSESSATTR_INIT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   INT32 _rtnSetSessionAttr::doit( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                   _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                                   INT16 w, INT64 *pContextID )
   {
      return SDB_OK ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnGetSessionAttr )

   _rtnGetSessionAttr::_rtnGetSessionAttr ()
   {
   }

   _rtnGetSessionAttr::~_rtnGetSessionAttr ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__RTNGETSESSATTR_INIT, "_rtnGetSessionAttr::init" )
   INT32 _rtnGetSessionAttr::init ( INT32 flags, INT64 numToSkip,
                                    INT64 numToReturn,
                                    const CHAR * pMatcherBuff,
                                    const CHAR * pSelectBuff,
                                    const CHAR * pOrderByBuff,
                                    const CHAR * pHintBuff )
   {
      return SDB_OK ;
   }

   INT32 _rtnGetSessionAttr::doit ( _pmdEDUCB * cb, _SDB_DMSCB * dmsCB,
                                    _SDB_RTNCB * rtnCB, _dpsLogWrapper * dpsCB,
                                    INT16 w, INT64 * pContextID )
   {
      return SDB_OK ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnTruncate)
   // PD_TRACE_DECLARE_FUNCTION( SDB__RTNTRUNCATE_INIT, "_rtnTruncate::init" )
   INT32 _rtnTruncate::init( INT32 flags, INT64 numToSkip,
                             INT64 numToReturn,
                             const CHAR *pMatcherBuff,
                             const CHAR *pSelectBuff,
                             const CHAR *pOrderByBuff,
                             const CHAR *pHintBuff )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNTRUNCATE_INIT ) ;
      try
      {
         BSONObj query( pMatcherBuff ) ;
         BSONElement ele = query.getField( FIELD_NAME_COLLECTION ) ;
         if ( String != ele.type() )
         {
            PD_LOG( PDERROR, "invalid collection name:%s",
                    query.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         _fullName = ele.valuestr() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__RTNTRUNCATE_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__RTNTRUNCATE_DOIT, ""_rtnTruncate::doit" )
   INT32 _rtnTruncate::doit( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                             _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                             INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNTRUNCATE_DOIT ) ;
      rc = rtnTruncCollectionCommand( _fullName,
                                      cb,
                                      dmsCB,
                                      dpsCB ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to truncate collection[%s], rc:%d",
                 _fullName, rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__RTNTRUNCATE_DOIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnAlterCollection )
   _rtnAlterCollection::_rtnAlterCollection()
   {

   }

   _rtnAlterCollection::~_rtnAlterCollection()
   {

   }

   const CHAR *_rtnAlterCollection::collectionFullName()
   {
      if ( !_alterObj.isEmpty() )
      {
         return _alterObj.getField( FIELD_NAME_NAME ).valuestrsafe() ;
      }
      else
      {
         return RTN_ALTER_TYPE_CL == _runner.getJob().getType() &&
                NULL != _runner.getJob().getName() ?
                _runner.getJob().getName() : NULL ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__RTNALTERCOLLECTION_INIT, "_rtnAlterCollection::init" )
   INT32 _rtnAlterCollection::init( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                                    const CHAR *pMatcherBuff,
                                    const CHAR *pSelectBuff,
                                    const CHAR *pOrderByBuff,
                                    const CHAR *pHintBuff )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNALTERCOLLECTION_INIT ) ;

      try
      {
         BSONObj obj( pMatcherBuff ) ;
         if ( obj.getField( FIELD_NAME_VERSION ).eoo() )
         {
            _alterObj = BSONObj( pMatcherBuff ) ;
            BSONElement options ;
            BSONElement clName = _alterObj.getField( FIELD_NAME_NAME ) ;
            if ( String != clName.type() )
            {
               PD_LOG( PDERROR, "invalid alter object:%s",
                       _alterObj.toString( FALSE, TRUE ).c_str() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            options = _alterObj.getField( FIELD_NAME_OPTIONS ) ;
            if ( Object != options.type() )
            {
               PD_LOG( PDERROR, "invalid alter object:%s",
                       _alterObj.toString( FALSE, TRUE ).c_str() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
         else
         {
            rc = _runner.init( BSONObj( pMatcherBuff ) ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to init rpc runner:%d", rc ) ;
               goto error ;
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected error happened:%s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__RTNALTERCOLLECTION_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__RTNALTERCOLLECTION_DOIT, "_rtnAlterCollection::doit" )
   INT32 _rtnAlterCollection::doit( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                    _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                                    INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNALTERCOLLECTION_DOIT ) ;
      if ( _alterObj.isEmpty() )
      {
         rc = _runner.run( cb, dpsCB ) ;

         if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
         {
            /// AUDIT
            PD_AUDIT_COMMAND( AUDIT_DDL, name(), AUDIT_OBJ_CL,
                              _runner.getJob().getName(), rc,
                              "Option:%s",
                              _runner.getJob().getJobObj().toString().c_str() ) ;
         }
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to run alter command:%d", rc ) ;
            goto error ;
         }
      }
      else if ( getFromService() != CMD_SPACE_SERVICE_SHARD )
      {
         rc = SDB_RTN_CMD_NO_SERVICE_AUTH ;
         PD_LOG( PDERROR, "this request should be from shard port" ) ;
         goto error ;
      }
      else
      {
         rc = _handleOldVersion( cb, dmsCB, rtnCB,
                                 dpsCB, w, pContextID ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to alter collection:%d", rc ) ;
            goto error ;
         }
      }
   done:
      PD_TRACE_EXITRC( SDB__RTNALTERCOLLECTION_DOIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__RTNALTERCOLLECTION__HANDLEOLDVERSION, "_rtnAlterCollection::_handleOldVersion" )
   INT32 _rtnAlterCollection::_handleOldVersion( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                                 _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                                                 INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNALTERCOLLECTION__HANDLEOLDVERSION ) ;
      BSONObj idxDef ;
      BSONObj options ;
      BSONElement ensureIndex  ;
      BSONElement shardingKey ;
      options = _alterObj.getField( FIELD_NAME_OPTIONS ).embeddedObject() ;
      shardingKey = options.getField( FIELD_NAME_SHARDINGKEY ) ;

      /// should get catalog info to do some more judgements.
      /// coord send the message with the newest catalog version
      if ( Object != shardingKey.type() )
      {
         PD_LOG( PDDEBUG, "no sharding key in the alter object, do noting." ) ;
         goto done ;
      }

      ensureIndex = options.getField( FIELD_NAME_ENSURE_SHDINDEX ) ;
      if ( Bool == ensureIndex.type() &&
           !ensureIndex.Bool() )
      {
         PD_LOG( PDDEBUG, "ensureShardingIndex is false, do nothing." ) ;
         goto done ;
      }

      idxDef = BSON( IXM_FIELD_NAME_KEY << shardingKey.embeddedObject()
                     << IXM_FIELD_NAME_NAME << IXM_SHARD_KEY_NAME
                     << "v"<<0 ) ;

      rc = rtnCreateIndexCommand( collectionFullName(),
                                  idxDef,
                                  cb, dmsCB, dpsCB, TRUE ) ;
      if ( SDB_IXM_REDEF == rc || SDB_IXM_EXIST_COVERD_ONE == rc )
      {
         /// sharding key index already exists.
         rc = SDB_OK ;
         goto done ;
      }
      else if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to create sharding key index:%d", rc ) ;
         goto error ;
      }
      else
      {
         /*
         the version which is from coord has been updated.
         we override the interface collectionFullName(). so
         the local catalog info will be autoly updated before
         this function is called.

         catalog info has been updated, clear local's info.
          it will download the last info when next request comes.
         catAgent *catAgent = sdbGetShardCB()->getCataAgent() ;
         catAgent->lock_w() ;
         catAgent->clear( collectionFullName() ) ;
         catAgent->release_w() ;
          notify other secondary nodes to clear catalog info.
         sdbGetClsCB()->invalidateCata( collectionFullName() ) ;
        */
      }
   done:
      PD_TRACE_EXITRC( SDB__RTNALTERCOLLECTION__HANDLEOLDVERSION, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnSyncDB )
   _rtnSyncDB::_rtnSyncDB()
   {
      _syncType = 1 ;
      _csName = NULL ;
      _block = FALSE ;
   }

   _rtnSyncDB::~_rtnSyncDB()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__RTNSYNCDB_INIT, "_rtnSyncDB::init" )
   INT32 _rtnSyncDB::init ( INT32 flags, INT64 numToSkip,
                            INT64 numToReturn,
                            const CHAR *pMatcherBuff,
                            const CHAR *pSelectBuff,
                            const CHAR *pOrderByBuff,
                            const CHAR *pHintBuff )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObj matcher( pMatcherBuff ) ;
         BSONElement e = matcher.getField( FIELD_NAME_DEEP ) ;
         if ( e.isNumber() )
         {
            _syncType = (INT32)e.numberInt() ;
         }
         else if ( e.isBoolean() )
         {
            _syncType = e.boolean() ? 1 : 0 ;
         }
         else if ( !e.eoo() )
         {
            PD_LOG( PDERROR, "Param[%s] is invalid in obj[%s]",
                    FIELD_NAME_DEEP, matcher.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         e = matcher.getField( FIELD_NAME_COLLECTIONSPACE ) ;
         if ( String == e.type() )
         {
            _csName = e.valuestr() ;
         }
         else if ( !e.eoo() )
         {
            PD_LOG( PDERROR, "Param[%s] is invalid in obj[%s]",
                    FIELD_NAME_COLLECTIONSPACE, matcher.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         e = matcher.getField( FIELD_NAME_BLOCK ) ;
         if ( Bool == e.type() )
         {
            _block = e.Bool() ? TRUE : FALSE ;
         }
         else if ( e.isNumber() )
         {
            _block = ( 0 == e.numberInt() ) ? FALSE : TRUE ;
         }
         else if ( !e.eoo() )
         {
            PD_LOG( PDERROR, "Param[%s] is invalid in obj[%s]",
                    FIELD_NAME_BLOCK, matcher.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__RTNSYNCDB_DOIT, "_rtnSyncDB::doit" )
   INT32 _rtnSyncDB::doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                            _SDB_RTNCB *rtnCB, _dpsLogWrapper *d,
                            INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNSYNCDB_DOIT ) ;

      rc = rtnSyncDB( cb, _syncType, _csName, _block ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to sync db: %d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNSYNCDB_DOIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnLoadCollectionSpace )
   _rtnLoadCollectionSpace::_rtnLoadCollectionSpace()
   {
      _csName = NULL ;
   }
   _rtnLoadCollectionSpace::~_rtnLoadCollectionSpace()
   {
   }

   INT32 _rtnLoadCollectionSpace::init( INT32 flags, INT64 numToSkip,
                                        INT64 numToReturn,
                                        const CHAR * pMatcherBuff,
                                        const CHAR * pSelectBuff,
                                        const CHAR * pOrderByBuff,
                                        const CHAR * pHintBuff )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObj matcher( pMatcherBuff ) ;
         BSONElement e = matcher.getField( FIELD_NAME_NAME ) ;
         if ( String == e.type() )
         {
            _csName = e.valuestr() ;
         }
         else
         {
            PD_LOG( PDERROR, "Param[%s] is invalid in obj[%s]",
                    FIELD_NAME_NAME, matcher.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnLoadCollectionSpace::doit( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                        _SDB_RTNCB * rtnCB,
                                        _dpsLogWrapper *dpsCB,
                                        INT16 w , INT64 *pContextID )
   {
      return rtnLoadCollectionSpace( _csName,
                                     pmdGetOptionCB()->getDbPath(),
                                     pmdGetOptionCB()->getIndexPath(),
                                     pmdGetOptionCB()->getLobPath(),
                                     pmdGetOptionCB()->getLobMetaPath(),
                                     dmsCB, FALSE ) ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnUnloadCollectionSpace )
   _rtnUnloadCollectionSpace::_rtnUnloadCollectionSpace()
   {
   }
   _rtnUnloadCollectionSpace::~_rtnUnloadCollectionSpace()
   {
   }

   INT32 _rtnUnloadCollectionSpace::doit( _pmdEDUCB * cb, _SDB_DMSCB * dmsCB,
                                          _SDB_RTNCB * rtnCB,
                                          _dpsLogWrapper * dpsCB,
                                          INT16 w, INT64 * pContextID )
   {
      return rtnUnloadCollectionSpace( _csName, cb, dmsCB ) ;
   }

}


