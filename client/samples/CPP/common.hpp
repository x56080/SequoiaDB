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

   
*******************************************************************************/
#ifndef COMMON_HPP__
#define COMMON_HPP__
#include <string>
#include "client.hpp"

using namespace sdbclient ;
using namespace std;
using namespace bson ;

namespace sample
{

    /* get a random number */
    UINT32 randNum() ;

    /* sleep x ms */
    void waiting(UINT32 milliseconds) ;

    /* connect to a given database */
    INT32 connectTo ( const CHAR *pHostName,
                      const CHAR *pServiceName,
                      const CHAR *pUser,
                      const CHAR *pPasswd,
                      sdb &connection ) ;
    /* get collection space, if the collection does not exist it will try to create
     * one */
    INT32 getCollectionSpace ( sdb &connection,
                               const CHAR *pCSName,
                               sdbCollectionSpace &collectionSpace ) ;

    /* get a collection, if the collection does not exist, it will try to create
     * one */
    INT32 getCollection ( sdb &connection,
                          const CHAR *pCollectionFullName,
                          sdbCollection &collection ) ;

    /* insert record into collection */
    INT32 insertRecord ( sdbCollection &collection,
                         BSONObj &obj ) ;

    CHAR *loadTag ( CHAR *pString ) ;

    BOOLEAN isComment ( CHAR *pString ) ;

    BOOLEAN loadJSON ( CHAR *pString, BSONObj &obj ) ;

    INT32 getIndexes ( sdbCollection &collection,
                       const CHAR *pIndexName,
                       sdbCursor &handle ) ;

    INT32 deleteRecords ( sdbCollection &collection,
                          BSONObj &cond,
                          BSONObj &hint ) ;

    INT32 createIndex ( sdbCollection &collection,
                        BSONObj &indexdef,
                        const CHAR *pIndexName,
                        BOOLEAN isUnique,
                        BOOLEAN isEnforced ) ;

    INT32 fetchRecords ( sdbCollection &collection,
                         BSONObj &condition,
                         BSONObj &selector,
                         BSONObj &orderBy,
                         BSONObj &hint,
                         INT64 skip,
                         INT64 numReturn ) ;


    void createEnglishRecord ( BSONObj &obj ) ;

    void createChineseRecord ( BSONObj &obj ) ;

    void createRecordList ( vector<BSONObj> &objlist, INT32 listSize ) ;

    string toJson( const BSONObj &b ) ;

    void displayRecord( sdbCursor &cursor ) ;

} // namespace sample
#endif
