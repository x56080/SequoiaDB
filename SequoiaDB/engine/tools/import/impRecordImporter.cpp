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

   Source File Name = impRecordImporter.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "impRecordImporter.hpp"
#include "ossUtil.h"
#include "pd.hpp"
#include "msgDef.h"

namespace import
{
   #define IMP_MAX_RECORDS_SIZE (SDB_MAX_MSG_LENGTH - 1024 * 1024 * 1)

   RecordImporter::RecordImporter(const string& hostname,
                                  const string& svcname,
                                  const string& user,
                                  const string& password,
                                  const string& csname,
                                  const string& clname,
                                  BOOLEAN useSSL,
                                  BOOLEAN enableTransaction,
                                  BOOLEAN allowKeyDuplication)
   : _hostname(hostname),
     _svcname(svcname),
     _user(user),
     _password(password),
     _csname(csname),
     _clname(clname),
     _useSSL(useSSL),
     _enableTransaction(enableTransaction),
     _allowKeyDuplication(allowKeyDuplication)
   {
      _connection = 0;
      _collectionSpace = 0;
      _collection = 0;
   }

   RecordImporter::~RecordImporter()
   {
      disconnect();
   }

   INT32 RecordImporter::connect()
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT(0 == _connection, "already connected");
      SDB_ASSERT(0 == _collectionSpace, "already get collection space");
      SDB_ASSERT(0 == _collection, "already get collection");

      if (_useSSL)
      {
         rc = sdbSecureConnect(_hostname.c_str(), _svcname.c_str(),
                               _user.c_str(), _password.c_str(), &_connection);
      }
      else
      {
         rc = sdbConnect(_hostname.c_str(), _svcname.c_str(),
                         _user.c_str(), _password.c_str(), &_connection);
      }

      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR,
                "Failed to connect to database %s:%s, rc = %d, usessl=%d",
                _hostname.c_str(), _svcname.c_str(), rc, _useSSL);
         goto error;
      }

      rc = sdbGetCollectionSpace(_connection, _csname.c_str(),
                                 &_collectionSpace);
      if (SDB_OK != rc)
      {
         if (SDB_DMS_CS_NOTEXIST == rc)
         {
            ossPrintf("collection space %s does not exist\n", _csname.c_str());
            PD_LOG(PDERROR, "collection space %s does not exist, rc = %d",
                   _csname.c_str(), rc);
         }
         else
         {
            PD_LOG(PDERROR, "failed to get collection space %s, rc = %d",
                   _csname.c_str(), rc);
         }
         goto error;
      }

      rc = sdbGetCollection1(_collectionSpace, _clname.c_str(), &_collection);
      if (SDB_OK != rc)
      {
         if (SDB_DMS_NOTEXIST == rc)
         {
            ossPrintf("collection %s does not exist.\n", _clname.c_str());
            PD_LOG(PDERROR, "collection %s does not exist, rc = %d",
                   _clname.c_str(), rc);
         }
         else if ( rc )
         {
            PD_LOG(PDERROR, "failed to get collection %s, rc = %d",
                   _clname.c_str(), rc);
         }
         goto error;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void RecordImporter::disconnect()
   {
      if (0 != _collection)
      {
         sdbReleaseCollection(_collection);
         _collection = 0;
      }

      if (0 != _collectionSpace)
      {
         sdbReleaseCS(_collectionSpace);
         _collectionSpace = 0;
      }

      if (0 != _connection)
      {
         sdbDisconnect(_connection);
         sdbReleaseConnection(_connection);
         _connection = 0;
      }
   }

   INT32 RecordImporter::import(bson* objs[], INT32 num)
   {
      INT32 rc = SDB_OK;
      INT32 flag = 0;

      SDB_ASSERT(NULL != objs, "objs can't be NULL");
      SDB_ASSERT(num > 0, "num must be greater than 0");
#ifdef _DEBUG
      for (INT32 i = 0; i < num; i++)
      {
         SDB_ASSERT(NULL != objs[i], "bson in objs can't be NULL");
      }
#endif

      if (_enableTransaction)
      {
         rc = sdbTransactionBegin(_connection);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to begin transaction, rc=%d", rc);
            goto error;
         }
      }

      if (_allowKeyDuplication)
      {
         flag |= FLG_INSERT_CONTONDUP;
      }

      rc = sdbBulkInsert(_collection, flag, objs, num);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to bulk insert, rc=%d", rc);
         // the transaction is rollbacked automatically
         goto error;
      }

      if (_enableTransaction)
      {
         rc = sdbTransactionCommit(_connection);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to commit transaction, rc=%d", rc);
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 RecordImporter::import(RecordArray* array)
   {
      INT32 rc = SDB_OK;
      INT32 size = 0;
      bson** records = NULL;

      SDB_ASSERT(NULL != array, "array can't be NULL");
      SDB_ASSERT(!array->empty(), "array can't be empty");

      size = array->size();
      records = array->array();

      if (array->bsonSize() <= IMP_MAX_RECORDS_SIZE)
      {
         rc = import(records, size);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to import records, size=%d, ,rc=%d",
                   array->bsonSize(), rc);
            goto error;
         }
      }
      else // the array is too large, so we split it
      {
         INT32 i = 0;
         INT32 start = 0;
         INT32 totalSize = 0;

         for (; i < size; i++)
         {
            bson* obj = records[i];
            SDB_ASSERT(NULL != obj, "obj can't be NULL");

            INT32 objSize = bson_size(obj);

            if (totalSize + objSize > IMP_MAX_RECORDS_SIZE)
            {
               rc = import(&records[start], i - start);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to import records, rc=%d", rc);
                  goto error;
               }
               start = i;
               totalSize = 0;
               continue;
            }

            totalSize += objSize;
         }

         // import last records in array
         if (totalSize > 0)
         {
            rc = import(&records[start], i - start);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to import records, rc=%d", rc);
               goto error;
            }
         }
      }

   done:
      return rc;
   error:
      goto done;
   }
}
