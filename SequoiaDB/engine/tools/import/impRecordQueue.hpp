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

   Source File Name = impRecordParser.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_RECORD_QUEUE_HPP__
#define IMP_RECORD_QUEUE_HPP__

#include "ossQueue.hpp"
#include "ossUtil.h"
#include "../client/bson/bson.h"
#include "pd.hpp"

namespace import
{
   class BsonArray: public SDBObject
   {
   public:
      BsonArray()
      {
         _array = NULL;
         _capacity = 0;
         _size = 0;
         _bsonSize = 0;
         _finished = FALSE;
      }

      ~BsonArray()
      {
         if (NULL != _array)
         {
            for (INT32 i = 0; i < _capacity; i++)
            {
               bson* obj = _array[i];
               if (NULL != obj)
               {
                  bson_destroy(obj);
                  SDB_OSS_FREE(obj);
               }
            }

            SDB_OSS_FREE(_array);
            _array = NULL;
         }

         _capacity = 0;
         _size = 0;
         _bsonSize = 0;
         _finished = FALSE;
      }

      INT32 init(INT32 capacity)
      {
         SDB_ASSERT(NULL == _array, "already inited");
         SDB_ASSERT(capacity > 0, "capacity must be greater than 0");

         _array = (bson**)SDB_OSS_MALLOC(sizeof(bson*) * capacity);
         if (NULL == _array)
         {
            return SDB_OOM;
         }
         ossMemset(_array, 0, sizeof(bson*) * capacity);
         _capacity = capacity;

         return SDB_OK;
      }

      inline INT32 capacity()
      {
         return _capacity;
      }

      inline INT32 size()
      {
         return _size;
      }

      inline INT64 bsonSize()
      {
         return _bsonSize;
      }

      inline BOOLEAN empty() const
      {
         return _size == 0;
      }

      inline BOOLEAN full() const
      {
         return _size == _capacity;
      }

      inline bson** array() const
      {
         return _array;
      }

      inline void finish()
      {
         SDB_ASSERT(NULL != _array, "already inited");
         
         _finished = TRUE;
      }

      inline void push(bson* obj)
      {
         SDB_ASSERT(NULL != _array, "already inited");
         SDB_ASSERT(NULL != obj, "obj can't be NULL");
         SDB_ASSERT(_size < _capacity, "_size out of range");
         SDB_ASSERT(!_finished, "already finished");

         _array[_size] = obj;
         _size++;

         _bsonSize += bson_size(obj) + sizeof(bson);
      }

      // return the bson* in position id
      inline bson* get(INT32 id)
      {
         SDB_ASSERT(NULL != _array, "already inited");
         SDB_ASSERT(id < _size, "id out of range");

         return _array[id];
      }

      // return the bson* in position id and erase it
      inline bson* pop(INT32 id)
      {
         SDB_ASSERT(NULL != _array, "already inited");
         SDB_ASSERT(id < _size, "id out of range");

         bson* obj = _array[id];
         _array[id] = NULL;
         return obj;
      }

   private:
      bson**   _array;
      INT32    _capacity;
      INT32    _size;
      INT64    _bsonSize;
      BOOLEAN  _finished;
   };

   typedef class BsonArray RecordArray;
   typedef class ossQueue<RecordArray*> RecordQueue;

   static inline INT32 getRecordArray(INT32 capacity,
                                    RecordArray** recordArray)
   {
      RecordArray* array = NULL;
      INT32 rc = SDB_OK;

      SDB_ASSERT(capacity >= 0, "capacity must >= 0");
      SDB_ASSERT(NULL != recordArray, "recordArray can't be NULL");

      array = SDB_OSS_NEW RecordArray();
      if (NULL == array)
      {
         rc = SDB_OOM;
         PD_LOG(PDERROR, "failed to alloc RecordArray, rc=%d", rc);
         goto error;
      }

      if (capacity > 0)
      {
         rc = array->init(capacity);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init RecordArray, rc=%d", rc);
            goto error;
         }
      }

      *recordArray = array;

   done:
      return rc;
   error:
      goto done;
   }

   static inline void freeRecordArray(RecordArray** recordArray)
   {
      SDB_ASSERT(NULL != recordArray, "recordArray can't be NULL");

      SDB_OSS_DEL(*recordArray);
      *recordArray = NULL;
   }

   static inline void freeRecord(bson* obj)
   {
      SDB_ASSERT(NULL != obj, "obj can't be NULL");

      bson_destroy(obj);
      SDB_OSS_FREE(obj);
   }
}

#endif /* IMP_RECORD_QUEUE_HPP__ */

