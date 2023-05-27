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

   Source File Name = fapMongoMessageDef.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who     Description
   ====== =========== ======= ==============================================
          2020/04/21  Ting YU Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef _SDB_MONGO_MESSAGE_DEF_HPP_
#define _SDB_MONGO_MESSAGE_DEF_HPP_

namespace fap
{

#define FAP_MONGO_FIELD_NAME_CMD             ".$cmd"
#define FAP_MONGO_FIELD_NAME_CMDQUERY        "$query"
#define FAP_MONGO_FIELD_NAME_DOCUMENTS       "documents"
#define FAP_MONGO_FIELD_NAME_DELETES         "deletes"
#define FAP_MONGO_FIELD_NAME_UPDATES         "updates"
#define FAP_MONGO_FIELD_NAME_INDEXES         "indexes"
#define FAP_MONGO_FIELD_NAME_INDEX           "index"
#define FAP_MONGO_FIELD_NAME_CURSORS         "cursors"
#define FAP_MONGO_FIELD_NAME_BATCHSIZE       "batchSize"
#define FAP_MONGO_FIELD_NAME_COLLECTION      "collection"
#define FAP_MONGO_FIELD_NAME_USERSINFO       "usersInfo"
#define FAP_MONGO_FIELD_NAME_QUERY           "query"
#define FAP_MONGO_FIELD_NAME_SORT            "sort"
#define FAP_MONGO_FIELD_NAME_NEW             "new"
#define FAP_MONGO_FIELD_NAME_FIELDS          "fields"
#define FAP_MONGO_FIELD_NAME_VALUE           "value"
#define FAP_MONGO_FIELD_NAME_N               "n"
#define FAP_MONGO_FIELD_NAME_LASTERROROBJECT "lastErrorObject"
#define FAP_MONGO_FIELD_NAME_UPDATEEXISTING  "updatedExisting"
#define FAP_MONGO_FIELD_NAME_UPSERT          "upsert"
#define FAP_MONGO_FIELD_NAME_TO              "to"
#define FAP_MONGO_FIELD_NAME_ID              "_id"
#define FAP_MONGO_FIELD_NAME_MAXTIMEMS       "maxTimeMS"
#define FAP_MONGO_FIELD_NAME_PIPELINE        "pipeline"

#define FAP_MONGO_FIELD_VALUE_NODEJS     "nodejs"
#define FAP_MONGO_FIELD_VALUE_MONGOSHELL "MongoDB Internal Client"
#define FAP_MONGO_FIELD_VALUE_JAVA       "mongo-java-driver"
#define FAP_MONGO_FIELD_VALUE_SCRAMSHA1  "SCRAM-SHA-1"

#define FAP_MONGO_EQUAL  "="
#define FAP_MONGO_COMMA  ","
#define FAP_MONGO_DOT    "."
#define FAP_MONGO_DOLLAR "$"

#define FAP_MONGO_FIELD_NAME_CODE   "code"
#define FAP_MONGO_FIELD_NAME_ERRMSG "errmsg"
#define FAP_MONGO_FIELD_NAME_OK     "ok"
#define FAP_MONGO_FIELD_NAME_ERR    "err"

#define FAP_MONGO_FIELD_NAME_USER       "user"
#define FAP_MONGO_FIELD_NAME_USERS      "users"
#define FAP_MONGO_FIELD_NAME_PWD        "pwd"
#define FAP_MONGO_FIELD_NAME_DIGESTPWD  "digestPassword"
#define FAP_MONGO_FIELD_NAME_PAYLOAD    "payload"
#define FAP_MONGO_FIELD_NAME_DONE       "done"
#define FAP_MONGO_FIELD_NAME_CONVERID   "conversationId"
#define FAP_MONGO_FIELD_NAME_MECHANISMS "mechanisms"

#define FAP_MONGO_FIELD_NAME_PATH                           "path"
#define FAP_MONGO_FIELD_NAME_INCLUDEARRAYINDEX              "includeArrayIndex"
#define FAP_MONGO_FIELD_NAME_PRESERVENULLANDEMPTYARRAYS     "preserveNullAndEmptyArrays"

#define FAP_MONGO_SASL_MSG_RANDOM   "r"
#define FAP_MONGO_SASL_MSG_SALT     "s"
#define FAP_MONGO_SASL_MSG_ITERATE  "i"
#define FAP_MONGO_SASL_MSG_USER     "n"
#define FAP_MONGO_SASL_MSG_PROOF    "p"
#define FAP_MONGO_SASL_MSG_CHANNEL  "c"
#define FAP_MONGO_SASL_MSG_VALUE    "v"
#define FAP_MONGO_SASL_MSG_ERROR    "e"

#define FAP_MONGO_OPERATOR_EQ        "$eq"
#define FAP_MONGO_OPERATOR_ET        "$et"
#define FAP_MONGO_OPERATOR_REPLACE   "$replace"
#define FAP_MONGO_OPERATOR_ISNULL    "$isnull"

// MongoDB aggregation pipeline stages
#define FAP_MONGO_AGGR_PIPELINE_STAGE_PREFIX          "$"
#define FAP_MONGO_AGGR_PIPELINE_STAGE_MATCH           FAP_MONGO_AGGR_PIPELINE_STAGE_PREFIX "match"
#define FAP_MONGO_AGGR_PIPELINE_STAGE_PROJECT         FAP_MONGO_AGGR_PIPELINE_STAGE_PREFIX "project"
#define FAP_MONGO_AGGR_PIPELINE_STAGE_GROUP           FAP_MONGO_AGGR_PIPELINE_STAGE_PREFIX "group"
#define FAP_MONGO_AGGR_PIPELINE_STAGE_SORT            FAP_MONGO_AGGR_PIPELINE_STAGE_PREFIX "sort"
#define FAP_MONGO_AGGR_PIPELINE_STAGE_SKIP            FAP_MONGO_AGGR_PIPELINE_STAGE_PREFIX "skip"
#define FAP_MONGO_AGGR_PIPELINE_STAGE_LIMIT           FAP_MONGO_AGGR_PIPELINE_STAGE_PREFIX "limit"
#define FAP_MONGO_AGGR_PIPELINE_STAGE_COUNT           FAP_MONGO_AGGR_PIPELINE_STAGE_PREFIX "count"
#define FAP_MONGO_AGGR_PIPELINE_STAGE_UNWIND          FAP_MONGO_AGGR_PIPELINE_STAGE_PREFIX "unwind"

}
#endif
