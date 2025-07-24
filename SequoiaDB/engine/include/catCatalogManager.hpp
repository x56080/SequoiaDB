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

   Source File Name = catCatalogManager.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/
#ifndef CATCATALOGUEMANAGER_HPP_
#define CATCATALOGUEMANAGER_HPP_

#include "pmd.hpp"
#include "catSplit.hpp"
#include "rtnContextBuff.hpp"
#include "utilCompressor.hpp"

using namespace bson ;

namespace engine
{

   class _dpsLogWrapper ;
   class sdbCatalogueCB ;
   class _SDB_DMSCB ;
   class _rtnAlterJob ;

   // create collection assign group type
   enum CAT_ASSIGNGROUP_TYPE
   {
      ASSIGN_FOLLOW     = 1,
      ASSIGN_RANDOM     = 2
   } ;

   #define CAT_MASK_CLNAME          0x00000001
   #define CAT_MASK_SHDKEY          0x00000002
   #define CAT_MASK_REPLSIZE        0x00000004
   #define CAT_MASK_SHDIDX          0x00000008
   #define CAT_MASK_SHDTYPE         0x00000010
   #define CAT_MASK_SHDPARTITION    0x00000020
   #define CAT_MASK_COMPRESSED      0x00000040
   #define CAT_MASK_ISMAINCL        0x00000080
   #define CAT_MASK_AUTOASPLIT      0x00000100
   #define CAT_MASK_AUTOREBALAN     0x00000200
   #define CAT_MASK_AUTOINDEXID     0x00000400
   #define CAT_MASK_COMPRESSIONTYPE 0x00000800

   struct _catCollectionInfo
   {
      const CHAR  *_pCLName ;
      BSONObj     _shardingKey ;
      INT32       _replSize ;
      BOOLEAN     _enSureShardIndex ;
      const CHAR  *_pShardingType ;
      INT32       _shardPartition ;
      BOOLEAN     _isHash ;
      BOOLEAN     _isSharding ;
      BOOLEAN     _isCompressed ;
      BOOLEAN     _isMainCL;
      BOOLEAN     _autoSplit ;
      BOOLEAN     _autoRebalance ;
      const CHAR * _gpSpecified ;
      INT32       _version ;
      INT32       _assignType ;
      BOOLEAN     _autoIndexId ;
      UTIL_COMPRESSOR_TYPE _compressorType ;

      std::vector<std::string>   _subCLList;

      _catCollectionInfo()
      {
         _pCLName             = NULL ;
         _replSize            = 1 ;
         _enSureShardIndex    = TRUE ;
         _pShardingType       = CAT_SHARDING_TYPE_HASH ;
         _shardPartition      = CAT_SHARDING_PARTITION_DEFAULT ;
         _isHash              = FALSE ;
         _isSharding          = FALSE ;
         _isCompressed        = FALSE ;
         _isMainCL            = FALSE ;
         _autoSplit           = FALSE ;
         _autoRebalance       = FALSE ;
         _gpSpecified         = NULL ;
         _version             = 0 ;
         _assignType          = ASSIGN_RANDOM ;
         _autoIndexId         = TRUE ;
         _compressorType      = UTIL_COMPRESSOR_INVALID ;
      }
   };
   typedef _catCollectionInfo catCollectionInfo ;

   struct _catCSInfo
   {
      const CHAR  *_pCSName ;
      INT32       _pageSize ;
      const CHAR  *_domainName ;
      INT32       _lobPageSize ;

      _catCSInfo()
      {
         _pCSName = NULL ;
         _pageSize = DMS_PAGE_SIZE_DFT ;
         _domainName = NULL ;
         _lobPageSize = DMS_DEFAULT_LOB_PAGE_SZ ;
      }

      BSONObj toBson()
      {
         BSONObjBuilder builder ;
         builder.append( CAT_COLLECTION_SPACE_NAME, _pCSName ) ;
         builder.append( CAT_PAGE_SIZE_NAME, _pageSize ) ;
         if ( _domainName )
         {
            builder.append( CAT_DOMAIN_NAME, _domainName ) ;
         }
         builder.append( CAT_LOB_PAGE_SZ_NAME, _lobPageSize ) ;
         return builder.obj() ;
      }
   } ;
   typedef _catCSInfo catCSInfo ;

   /*
      catCatalogueManager define
   */
   class catCatalogueManager : public SDBObject
   {
   public:
      catCatalogueManager() ;

      INT32 init() ;
      INT32 fini() ;

      void  attachCB( _pmdEDUCB *cb ) ;
      void  detachCB( _pmdEDUCB *cb ) ;

      INT32 processMsg( const NET_HANDLE &handle, MsgHeader *pMsg ) ;

      INT32 active() ;
      INT32 deactive() ;

   // message process functions
   protected:
      INT32 processCommandMsg( const NET_HANDLE &handle, MsgHeader *pMsg,
                               BOOLEAN writable ) ;

      INT32 processCmdCreateCS( const CHAR *pQuery,
                                rtnContextBuf &ctxBuf ) ;
      INT32 processCmdSplit( const CHAR *pQuery,
                             INT32 opCode,
                             rtnContextBuf &ctxBuf ) ;
      INT32 processCmdQuerySpaceInfo( const CHAR *pQuery,
                                      rtnContextBuf &ctxBuf ) ;
      INT32 processQueryCatalogue ( const NET_HANDLE &handle,
                                    MsgHeader *pMsg ) ;
      INT32 processQueryTask ( const NET_HANDLE &handle, MsgHeader *pMsg ) ;
      INT32 processCmdCrtProcedures( void *pMsg ) ;
      INT32 processCmdRmProcedures( void *pMsg ) ;
      INT32 processCmdCreateDomain ( const CHAR *pQuery ) ;
      INT32 processCmdDropDomain ( const CHAR *pQuery ) ;
      INT32 processCmdAlterDomain ( const CHAR *pQuery ) ;

   // tool functions
   protected:
      void  _fillRspHeader( MsgHeader *rspMsg, const MsgHeader *reqMsg ) ;

      INT32 _createCS( BSONObj & createObj, UINT32 &groupID ) ;

      INT32 _checkCSObj( const BSONObj &infoObj,
                         catCSInfo &csInfo ) ;

      INT32 _assignGroup( vector< UINT32 > *pGoups, UINT32 &groupID ) ;

   private:
      INT16 _majoritySize() ;

      INT32 _buildAlterGroups( const BSONObj &domain,
                               const BSONElement &ele,
                               BSONObjBuilder &builder ) ;

   private:
      sdbCatalogueCB       *_pCatCB;
      _SDB_DMSCB           *_pDmsCB;
      _dpsLogWrapper       *_pDpsCB;
      pmdEDUCB             *_pEduCB;
      clsTaskMgr           _taskMgr ;

   } ;
}

#endif // CATCATALOGUEMANAGER_HPP_

