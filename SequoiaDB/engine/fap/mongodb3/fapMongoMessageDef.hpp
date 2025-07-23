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

#define FAP_MONGO_FIELD_NAME_CMD         ".$cmd"
#define FAP_MONGO_FIELD_NAME_DOCUMENTS   "documents"
#define FAP_MONGO_FIELD_NAME_DELETES     "deletes"
#define FAP_MONGO_FIELD_NAME_UPDATES     "updates"
#define FAP_MONGO_FIELD_NAME_INDEXES     "indexes"
#define FAP_MONGO_FIELD_NAME_INDEX       "index"
#define FAP_MONGO_FIELD_NAME_CURSORS     "cursors"
#define FAP_MONGO_FIELD_NAME_BATCHSIZE   "batchSize"
#define FAP_MONGO_FIELD_NAME_COLLECTION  "collection"
#define FAP_MONGO_FIELD_VALUE_NODEJS     "nodejs"
#define FAP_MONGO_FIELD_VALUE_MONGOSHELL "MongoDB Internal Client"
#define FAP_MONGO_FIELD_VALUE_JAVA       "mongo-java-driver"

#define FAP_MONGO_EQUAL "="
#define FAP_MONGO_COMMA ","
#define FAP_MONGO_DOT   "."

#define FAP_MONGO_FIELD_NAME_CODE   "code"
#define FAP_MONGO_FIELD_NAME_ERRMSG "errmsg"
#define FAP_MONGO_FIELD_NAME_OK     "ok"

#define FAP_MONGO_FIELD_NAME_USER      "user"
#define FAP_MONGO_FIELD_NAME_USERS     "users"
#define FAP_MONGO_FIELD_NAME_PWD       "pwd"
#define FAP_MONGO_FIELD_NAME_DIGESTPWD "digestPassword"
#define FAP_MONGO_FIELD_NAME_PAYLOAD   "payload"
#define FAP_MONGO_FIELD_NAME_DONE      "done"
#define FAP_MONGO_FIELD_NAME_CONVERID  "conversationId"

#define FAP_MONGO_SASL_MSG_RANDOM   "r"
#define FAP_MONGO_SASL_MSG_SALT     "s"
#define FAP_MONGO_SASL_MSG_ITERATE  "i"
#define FAP_MONGO_SASL_MSG_USER     "n"
#define FAP_MONGO_SASL_MSG_PROOF    "p"
#define FAP_MONGO_SASL_MSG_CHANNEL  "c"
#define FAP_MONGO_SASL_MSG_VALUE    "v"
#define FAP_MONGO_SASL_MSG_ERROR    "e"
}
#endif
