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

   Source File Name = catCommon.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/07/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CAT_COMMON_HPP__
#define CAT_COMMON_HPP__

#include "core.hpp"
#include "pd.hpp"
#include "oss.hpp"
#include "ossErr.h"
#include "../bson/bson.h"
#include "catDef.hpp"

using namespace bson ;

namespace engine
{

   class _SDB_DMSCB ;
   class _dpsLogWrapper ;

   /* Check group name is valid */
   INT32 catGroupNameValidate ( const CHAR *pName, BOOLEAN isSys = FALSE ) ;

   /* Check domain name is valid */
   INT32 catDomainNameValidate( const CHAR *pName ) ;


   /* extract options of domain. when builder is NULL only check validation */
   INT32 catDomainOptionsExtract( const BSONObj &options,
                                  pmdEDUCB *cb,
                                  BSONObjBuilder *builder = NULL,
                                  vector< string > *pVecGroups = NULL ) ;

   /* Split collection full name to cs name and cl name */
   INT32 catResolveCollectionName( const CHAR *pInput, UINT32 inputLen,
                                   CHAR *pSpaceName, UINT32 spaceNameSize,
                                   CHAR *pCollectionName,
                                   UINT32 collectionNameSize ) ;

   /* Query and return result */
   INT32 catQueryAndGetMore ( MsgOpReply **ppReply,
                              const CHAR *collectionName,
                              BSONObj &selector,
                              BSONObj &matcher,
                              BSONObj &orderBy,
                              BSONObj &hint,
                              SINT32 flags,
                              pmdEDUCB *cb,
                              SINT64 numToSkip,
                              SINT64 numToReturn ) ;

   /* Query and get one object */
   INT32 catGetOneObj( const CHAR *collectionName,
                       const BSONObj &selector,
                       const BSONObj &matcher,
                       const BSONObj &hint,
                       pmdEDUCB *cb,
                       BSONObj &obj ) ;

   /* Collection[CAT_NODE_INFO_COLLECTION] functions: */
   INT32 catGetGroupObj( const CHAR *groupName,
                         BOOLEAN dataGroupOnly,
                         BSONObj &obj, pmdEDUCB *cb ) ;
   INT32 catGetGroupObj( UINT32 groupID, BSONObj &obj, pmdEDUCB *cb ) ;
   INT32 catGetGroupObj( UINT16 nodeID, BSONObj &obj, pmdEDUCB *cb ) ;

   INT32 catGroupCheck( const CHAR *groupName, BOOLEAN &exist, pmdEDUCB *cb ) ;
   INT32 catServiceCheck( const CHAR *hostName, const CHAR *serviceName,
                          BOOLEAN &exist, pmdEDUCB *cb ) ;

   INT32 catGroupID2Name( UINT32 groupID, string &groupName, pmdEDUCB *cb ) ;
   INT32 catGroupName2ID( const CHAR *groupName, UINT32 &groupID,
                          pmdEDUCB *cb ) ;

   INT32 catGroupCount( INT64 & count, pmdEDUCB * cb ) ;

   /* Collection[CAT_DOMAIN_COLLECTION] functions: */
   INT32 catGetDomainObj( const CHAR *domainName, BSONObj &obj, pmdEDUCB *cb ) ;
   INT32 catDomainCheck( const CHAR *domainName, BOOLEAN &exist, pmdEDUCB *cb ) ;

   INT32 catGetDomainGroups( const BSONObj &domain,
                             map<string, UINT32> &groups ) ;
   INT32 catGetDomainGroups( const BSONObj &domain,
                             vector< UINT32 > &groupIDs ) ;
   INT32 catAddGroup2Domain( const CHAR *domainName, const CHAR *groupName,
                             INT32 groupID, pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                             _dpsLogWrapper *dpsCB, INT16 w ) ;
   /*
      Note: domainName == NULL, while del group from all domain
   */
   INT32 catDelGroupFromDomain( const CHAR *domainName, const CHAR *groupName,
                                UINT32 groupID, pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                _dpsLogWrapper *dpsCB, INT16 w ) ;

   /* Collection[CAT_COLLECTION_SPACE_COLLECTION] functions: */
   INT32 catAddCL2CS( const CHAR *csName, const CHAR *clName,
                      pmdEDUCB *cb, _SDB_DMSCB * dmsCB,
                      _dpsLogWrapper * dpsCB, INT16 w ) ;
   INT32 catDelCLFromCS( const CHAR *csName, const CHAR *clName,
                         pmdEDUCB *cb, _SDB_DMSCB * dmsCB,
                         _dpsLogWrapper * dpsCB,
                         INT16 w ) ;
   INT32 catRestoreCS( const CHAR *csName, const BSONObj &oldInfo,
                       pmdEDUCB *cb, _SDB_DMSCB * dmsCB,
                       _dpsLogWrapper * dpsCB,
                       INT16 w ) ;
   INT32 catCheckSpaceExist( const char *pSpaceName,
                             BOOLEAN &isExist,
                             BSONObj &obj,
                             pmdEDUCB *cb ) ;
   INT32 catGetDomainCSs( const BSONObj &domain, pmdEDUCB *cb,
                          vector< string > &collectionSpaces ) ;

   /* Collection[CAT_COLLECTION_INFO_COLLECTION] functions: */
   INT32 catRemoveCL( const CHAR *clFullName, pmdEDUCB *cb, _SDB_DMSCB * dmsCB,
                      _dpsLogWrapper * dpsCB, INT16 w );

   INT32 catCheckCollectionExist( const CHAR *pCollectionName,
                                  BOOLEAN &isExist,
                                  BSONObj &obj,
                                  pmdEDUCB *cb ) ;

   INT32 catUpdateCatalog( const CHAR *clFullName, const BSONObj &cataInfo,
                           pmdEDUCB *cb, INT16 w ) ;

   INT32 catGetCSGroupsFromCLs( const CHAR *csName, pmdEDUCB *cb,
                                vector< UINT32 > &groups,
                                BOOLEAN includeSubCLGroups = FALSE ) ;

   /* Collection[CAT_TASK_INFO_COLLECTION] functions: */
   INT32 catAddTask( BSONObj & taskObj, pmdEDUCB *cb, INT16 w ) ;
   INT32 catGetTask( UINT64 taskID, BSONObj &obj, pmdEDUCB *cb ) ;
   INT32 catGetTaskStatus( UINT64 taskID, INT32 &status, pmdEDUCB *cb ) ;
   INT32 catUpdateTaskStatus( UINT64 taskID, INT32 status, pmdEDUCB *cb,
                              INT16 w ) ;
   INT64 catGetMaxTaskID( pmdEDUCB *cb ) ;
   INT32 catRemoveTask( UINT64 taskID, pmdEDUCB *cb, INT16 w ) ;
   INT32 catRemoveTask( BSONObj &match, pmdEDUCB *cb, INT16 w ) ;
   INT32 catGetCSGroupsFromTasks( const CHAR *csName, pmdEDUCB *cb,
                                  vector< UINT32 > &groups ) ;

   /* Collection[CAT_HISTORY_COLLECTION] functions */
   INT32 catGetBucketVersion( const CHAR *pCLName, pmdEDUCB *cb ) ;
   INT32 catSaveBucketVersion( const CHAR *pCLName, INT32 version,
                               pmdEDUCB *cb, INT16 w ) ;

   /* Collection[CAT_SYSDCBASE_COLLECTION_NAME] functions */
   INT32 catCheckBaseInfoExist( const CHAR *pTypeStr,
                                BOOLEAN &isExist,
                                BSONObj &obj,
                                pmdEDUCB *cb ) ;
   INT32 catUpdateBaseInfoAddr( const CHAR *pAddr,
                                BOOLEAN self,
                                pmdEDUCB *cb,
                                INT16 w ) ;
   INT32 catEnableImage( BOOLEAN enable, pmdEDUCB *cb, INT16 w,
                         _SDB_DMSCB *dmsCB, _dpsLogWrapper * dpsCB ) ;

   INT32 catUpdateDCStatus( const CHAR *pField, BOOLEAN status,
                            pmdEDUCB *cb, INT16 w,
                            _SDB_DMSCB *dmsCB, _dpsLogWrapper *dpsCB ) ;

   /* Other Tools */
   INT32 catRemoveCLEx( const CHAR *clFullName,  pmdEDUCB *cb,
                        _SDB_DMSCB * dmsCB, _dpsLogWrapper * dpsCB, INT16 w,
                        BOOLEAN delSubCL = FALSE, INT32 version = -1 ) ;
   INT32 catRemoveCSEx( const CHAR *csName, pmdEDUCB *cb,
                        _SDB_DMSCB * dmsCB, _dpsLogWrapper * dpsCB, INT16 w ) ;

   INT32 catPraseFunc( const BSONObj &func, BSONObj &parsed ) ;

   INT32 catUnlinkCL( const CHAR *mainCLName, const CHAR *subCLName,
                      pmdEDUCB *cb, _SDB_DMSCB * dmsCB, _dpsLogWrapper * dpsCB,
                      INT16 w, std::vector<UINT32>  &groupList );

   INT32 catLinkCL( const CHAR *mainCLName, const CHAR *subCLName,
                    BSONObj &boLowBound, BSONObj &boUpBound,
                    pmdEDUCB *cb, _SDB_DMSCB * dmsCB, _dpsLogWrapper * dpsCB,
                    INT16 w, std::vector<UINT32>  &groupList );

   INT32 catTestAndCreateCL( const CHAR *pCLFullName, pmdEDUCB *cb,
                             _SDB_DMSCB *dmsCB, _dpsLogWrapper *dpsCB,
                             BOOLEAN sys = TRUE ) ;

   INT32 catTestAndCreateIndex( const CHAR *pCLFullName,
                                const BSONObj &indexDef,
                                pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                _dpsLogWrapper *dpsCB, BOOLEAN sys = TRUE) ;

   UINT32 catCalcBucketID( const CHAR *pData, UINT32 length,
                           UINT32 bucketSize = CAT_BUCKET_SIZE ) ;

}


#endif //CAT_COMMON_HPP__

