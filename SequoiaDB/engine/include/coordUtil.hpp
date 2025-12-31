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

   Source File Name = coordUtil.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/13/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef COORD_UTIL_HPP__
#define COORD_UTIL_HPP__

#include "coordDef.hpp"
#include "coordCommon.hpp"
#include "coordResource.hpp"
#include "../bson/bson.h"
#include "utilResult.hpp"

using namespace bson ;

namespace engine
{
   class _pmdEDUCB ;

   void  coordBuildFailedNodeReply( coordResource *pResource,
                                    ROUTE_RC_MAP &failedNodes,
                                    BSONObjBuilder &builder ) ;

   BSONObj coordBuildErrorObj( coordResource *pResource,
                               INT32 &flag,
                               _pmdEDUCB *cb,
                               ROUTE_RC_MAP *pFailedNodes,
                               UINT32 sucNum = 0 ) ;

   void    coordBuildErrorObj( coordResource *pResource,
                               INT32 &flag,
                               _pmdEDUCB *cb,
                               ROUTE_RC_MAP *pFailedNodes,
                               BSONObjBuilder &builder,
                               UINT32 sucNum = 0 ) ;

   void    coordSetResultInfo( INT32 flag,
                               ROUTE_RC_MAP &failedNodes,
                               utilWriteResult *pResult ) ;

   INT32 coordGetGroupsFromObj( const BSONObj &obj,
                                CoordGroupList &groupLst,
                                const CHAR* fieldName ) ;

   INT32 coordGetGroupsFromObj( const BSONObj &obj,
                                CoordGroupList &groupLst ) ;

   INT32 coordGetFailedGroupsFromObj( const BSONObj &obj,
                                      CoordGroupList &groupLst ) ;

   INT32 coordParseGroupList( coordResource *pResource,
                              _pmdEDUCB *cb,
                              const BSONObj &obj,
                              CoordGroupList &groupList,
                              BSONObj *pNewObj = NULL,
                              BOOLEAN strictCheck = FALSE ) ;

   INT32 coordParseGroupList( coordResource *pResource,
                              _pmdEDUCB *cb,
                              MsgOpQuery *pMsg,
                              FILTER_BSON_ID filterObjID,
                              CoordGroupList &groupList,
                              BOOLEAN strictCheck = FALSE ) ;

   INT32 coordGroupList2GroupPtr( coordResource *pResource,
                                  _pmdEDUCB *cb,
                                  CoordGroupList &groupList,
                                  GROUP_VEC & groupPtrs ) ;

   INT32 coordGroupList2GroupPtr( coordResource *pResource,
                                  _pmdEDUCB *cb,
                                  CoordGroupList &groupList,
                                  CoordGroupMap &groupMap,
                                  BOOLEAN reNew ) ;

   void  coordGroupPtr2GroupList( GROUP_VEC &groupPtrs,
                                  CoordGroupList &groupList ) ;

   INT32 coordGetGroupNodes( coordResource *pResource,
                             _pmdEDUCB *cb,
                             const BSONObj &filterObj,
                             NODE_SEL_STY emptyFilterSel,
                             CoordGroupList &groupList,
                             SET_ROUTEID &nodes,
                             BSONObj *pNewObj = NULL,
                             BOOLEAN strictCheck = FALSE ) ;

   INT32 coordGetCLDataSource( const CHAR *collection,
                               pmdEDUCB *cb,
                               coordResource *pResource,
                               BOOLEAN &isDataSourceCL,
                               BOOLEAN &isHighErrLevel ) ;

   INT32 coordRemoveFailedGroup( CoordGroupList &groupLst,
                                 BOOLEAN &hasFailedGroup,
                                 const vector<BSONObj> &replyObjs ) ;

   INT32 coordValidateSubCLBounds( const clsCatalogSet &mainCLCataSet,
                                   const BSONObj &obj,
                                   BOOLEAN hasUpdateCatalog,
                                   BOOLEAN strict = TRUE,
                                   BOOLEAN *pExistCLBounds = NULL ) ;

   INT32 coordRemoveSubCLBounds( BSONObj &obj ) ;

   BSONObj coordBuildSubCLIntoObj( const BSONObj &obj,
                                   const CoordSubCLlist &subCLList ) ;
}

#endif // COORD_UTIL_HPP__

