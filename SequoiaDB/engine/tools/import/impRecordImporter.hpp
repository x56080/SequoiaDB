/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = impRecordImporter.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_RECORD_IMPORTER_HPP_
#define IMP_RECORD_IMPORTER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "impRecordQueue.hpp"
#include "../client/client.h"
#include <string>

using namespace std;

namespace import
{
   class RecordImporter: public SDBObject
   {
   public:
      RecordImporter(const string& hostname,
                     const string& svcname,
                     const string& user,
                     const string& password,
                     const string& csname,
                     const string& clname,
                     BOOLEAN useSSL = FALSE,
                     BOOLEAN enableTransaction = FALSE,
                     BOOLEAN allowKeyDuplication = TRUE);
      ~RecordImporter();
      INT32 connect();
      void disconnect();
      INT32 import(bson* objs[], INT32 num);
      INT32 import(RecordArray* array);

   private:
      string   _hostname;
      string   _svcname;
      string   _user;
      string   _password;
      string   _csname;
      string   _clname;
      BOOLEAN  _useSSL;
      BOOLEAN  _enableTransaction;
      BOOLEAN  _allowKeyDuplication;

      // db handle
      sdbConnectionHandle  _connection;
      sdbCSHandle          _collectionSpace;
      sdbCollectionHandle  _collection;
   };
}

#endif /* IMP_RECORD_IMPORTER_HPP_ */
