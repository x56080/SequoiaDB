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

   Source File Name = vessel_test.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/vesselImpl.h"
#include "vessel/ISession.h"
#include "vessel/requestContext.h"
#include <gtest/gtest.h>
#include "ossUtil.hpp"
#include "vessel/IRedoLogger.h"
#include "dpsLogRecord.hpp"
#include "vessel/ICursor.h"
#include "vessel/listCollectionSpaceDef.h"
#include "vessel/listCollectionsDef.h"
#include "vessel/logRecordContext.h"
#include "dpsLogRecord.hpp"

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
const CHAR *DATA_PATH = "/tmp/vessel_test";

using namespace engine::vessel;
using namespace engine;

class vesseltest : public testing::Test
{
   public:
   static void SetUpTestCase()
   {
      fs::path testPath(DATA_PATH);
      fs::remove_all(testPath);
      fs::create_directory(testPath);
   }

   static void TearDownTestCase()
   {
      fs::path testPath(DATA_PATH);
      fs::remove_all(testPath);
   }

   virtual void SetUp()
   {
      fs::path testPath(DATA_PATH);
      fs::remove_all(testPath);
      fs::create_directory(testPath);
   }
};

class test_session : public ::engine::vessel::ISession
{
   public:
      test_session():
      _id(0)
      {}
      virtual ~test_session(){}

   public:
      virtual UINT64 getSessionID()const
      {
         return 0;
      }

      virtual void setLastError(INT32 rc, const CHAR *fmt, ...)
      {
         return ;
      }

      virtual void clearLastError()
      {
         return;
      }

      virtual BOOLEAN quit()const
      {
         return FALSE;
      }

      virtual BOOLEAN nowait()const
      {
         return FALSE;
      }
   private:
      UINT32 _id;
};

class test_logger : public IRedoLogger
{
   public:
      test_logger(){
         _lsn = 0;
      }
      virtual ~test_logger(){}

      virtual INT32 log(::engine::vessel::ISession *session,
                           const _dpsLogRecord *record,
                           DPS_LSN_OFFSET *lsn)
      {
         if (NULL != lsn)
         {
            *lsn = _lsn;
         }
         _lsn += record->alignedLen();
         return SDB_OK;
      }

         /// allocate lsn and log buffer.
         virtual INT32 prepare(::engine::vessel::ISession *session,
                               logRecordContext *context)
         {
            if (context->prepared())
            {
               return SDB_INVALIDARG;
            }
            context->getHead()._lsn = _lsn;
            _lsn += context->getHead()._length;
            return SDB_OK;
         }

         virtual INT32 pushLogRecordElement(::engine::vessel::ISession *session,
                                            logRecordContext *context,
                                            DPS_TAG tag,
                                            UINT32 len,
                                            const void *value)
         {
            if (!context->prepared())
            {
              return SDB_INVALIDARG;
            }
            return SDB_OK;
         }

         virtual INT32 commit(::engine::vessel::ISession *session,
                              logRecordContext *context)
         {
            if (!context->prepared())
            {
               return SDB_INVALIDARG;
            }
            return SDB_OK;
         }

         /// do not commit log after commit.
         virtual INT32 abort(::engine::vessel::ISession *session,
                             logRecordContext *context)
         {
            if (!context->prepared())
            {
               return SDB_INVALIDARG;
            }
            return SDB_OK;
            
         }

         virtual INT32 pushMaxFileLSN(::engine::vessel::ISession *session,
                                      DPS_LSN_OFFSET lsn)
         {
            return SDB_OK;
         }

   private:
      UINT64 _lsn;
};

TEST_F(vesseltest, test1)
{
   INT32 rc = SDB_OK;
   test_logger logger;
   vesselImpl db;
   outerResource resource;
   resource.logger = &logger; 
   test_session session;
   openDBOptions options;
   createCSOptions csOptions;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobMetaPath = DATA_PATH;
   options.path.lobPath = DATA_PATH;

   UINT32 count = 0;

   db.setup(resource);
   rc = db.open(&session, options);
   ASSERT_EQ(SDB_OK, rc);

   csOptions.logicalID = 1;
   rc = db.createCollectionSpace(&session, "foo", csOptions);
   ASSERT_EQ(SDB_OK, rc);
   rc = db.fastGetCollectionSpaceCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(1, count);

   csOptions.logicalID = 2;
   rc = db.createCollectionSpace(&session, "foo", csOptions);
   ASSERT_EQ(SDB_DMS_CS_EXIST, rc);

   csOptions.logicalID = 1;
   rc = db.createCollectionSpace(&session, "foo1", csOptions);
   ASSERT_EQ(SDB_DMS_CS_EXIST, rc);

   csOptions.logicalID = 2;
   rc = db.createCollectionSpace(&session, "bar", csOptions);
   ASSERT_EQ(SDB_OK, rc);
   rc = db.fastGetCollectionSpaceCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(2, count);
   db.close(&session, closeDBOptions());
}

/*
TEST_F(vesseltest, test2)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   test_session session;
   openDBOptions options;
   createCSOptions csOptions;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobMetaPath = DATA_PATH;
   options.path.lobPath = DATA_PATH;
   rc = db.open(&session, options);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < MAX_SPACE_COUNT; ++i)
   {
      CHAR buf[6] = {0};
      ossSnprintf(buf, 6, "%d", i);
      rc = db.createCollectionSpace(&session, buf, i, csOptions);
      ASSERT_EQ(SDB_OK, rc);
   }

   CHAR buf[6] = {0};
   ossSnprintf(buf, 6, "%d", MAX_SPACE_COUNT);
   rc = db.createCollectionSpace(&session, buf, MAX_SPACE_COUNT, csOptions);
   ASSERT_EQ(SDB_DMS_SU_OUTRANGE, rc);


   db.close(&session, closeDBOptions());
}*/

TEST_F(vesseltest, test3)
{
   INT32 rc = SDB_OK;
   test_logger logger;
   outerResource resource;
   resource.logger = &logger; 
   vesselImpl db;
   test_session session;
   openDBOptions options;
   createCSOptions csOptions;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobMetaPath = DATA_PATH;
   options.path.lobPath = DATA_PATH;
   UINT32 count = 0;

   db.setup(resource);
   rc = db.open(&session, options);
   ASSERT_EQ(SDB_OK, rc);

   csOptions.logicalID = 1;
   rc = db.createCollectionSpace(&session, "foo", csOptions);
   ASSERT_EQ(SDB_OK, rc);

   csOptions.logicalID = 2;
   rc = db.createCollectionSpace(&session, "bar", csOptions);
   ASSERT_EQ(SDB_OK, rc);
   db.close(&session, closeDBOptions());

   rc = db.open(&session, options);
   ASSERT_EQ(SDB_OK, rc);
   rc = db.fastGetCollectionSpaceCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(2, count);
   db.close(&session, closeDBOptions());
}

TEST_F(vesseltest, test4)
{
   INT32 rc = SDB_OK;
   test_logger logger;
   outerResource resource;
   resource.logger = &logger; 
   vesselImpl db;
   test_session session;
   openDBOptions options;
   createCSOptions csOptions;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobMetaPath = DATA_PATH;
   options.path.lobPath = DATA_PATH;
   UINT32 count = 0;
   slice content;
   const listCollectionSpaceRecord *record = NULL;
   ICursor c;

   db.setup(resource);
   rc = db.open(&session, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.listCollectionSpace(&session, NULL, &c);
   ASSERT_EQ(SDB_OK, rc);

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_VESSEL_END_OF_CURSOR, rc);
   c.close();

   csOptions.logicalID = 1;
   rc = db.createCollectionSpace(&session, "foo1", csOptions);
   ASSERT_EQ(SDB_OK, rc);

   csOptions.logicalID = 2;
   rc = db.createCollectionSpace(&session, "foo2", csOptions);
   ASSERT_EQ(SDB_OK, rc);

   csOptions.logicalID = 3;
   rc = db.createCollectionSpace(&session, "foo3", csOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.listCollectionSpace(&session, NULL, &c);
   ASSERT_EQ(SDB_OK, rc);

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_TRUE(content.valid());
   record = (const listCollectionSpaceRecord *)content.data();
   ASSERT_EQ(0, ossStrcmp("foo1", record->name));
   ASSERT_EQ(1, record->logicalID);

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_TRUE(content.valid());
   record = (const listCollectionSpaceRecord *)content.data();
   ASSERT_EQ(0, ossStrcmp("foo2", record->name));
   ASSERT_EQ(2, record->logicalID);

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_TRUE(content.valid());
   record = (const listCollectionSpaceRecord *)content.data();
   ASSERT_EQ(0, ossStrcmp("foo3", record->name));
   ASSERT_EQ(3, record->logicalID);

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_VESSEL_END_OF_CURSOR, rc);
   c.close();


   db.close(&session, closeDBOptions());

   rc = db.open(&session, options);
   ASSERT_EQ(SDB_OK, rc);
   rc = db.fastGetCollectionSpaceCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(3, count);

   rc = db.listCollectionSpace(&session, NULL, &c);
   ASSERT_EQ(SDB_OK, rc);

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_TRUE(content.valid());
   record = (const listCollectionSpaceRecord *)content.data();
   ASSERT_EQ(0, ossStrcmp("foo1", record->name));
   ASSERT_EQ(1, record->logicalID);

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_TRUE(content.valid());
   record = (const listCollectionSpaceRecord *)content.data();
   ASSERT_EQ(0, ossStrcmp("foo2", record->name));
   ASSERT_EQ(2, record->logicalID);

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_TRUE(content.valid());
   record = (const listCollectionSpaceRecord *)content.data();
   ASSERT_EQ(0, ossStrcmp("foo3", record->name));
   ASSERT_EQ(3, record->logicalID);

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_VESSEL_END_OF_CURSOR, rc);
   c.close();

   db.close(&session, closeDBOptions());
}

///create cl
TEST_F(vesseltest, test5)
{
   INT32 rc = SDB_OK;
   test_logger logger;
   vesselImpl db;
   outerResource resource;
   resource.logger = &logger; 
   test_session session;
   openDBOptions options;
   createCSOptions csOptions;
   createCLOptions clOptions;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobMetaPath = DATA_PATH;
   options.path.lobPath = DATA_PATH;
   ICursor cursor;
   slice slice;
   const listCollectionsRecord *record = NULL;


   UINT32 count = 0;

   db.setup(resource);
   rc = db.open(&session, options);
   ASSERT_EQ(SDB_OK, rc);

   csOptions.logicalID = 1;
   rc = db.createCollectionSpace(&session, "foo", csOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollection(&session, csOptions.logicalID, "bar1", 1, clOptions);
   ASSERT_EQ(SDB_OK, rc);
   rc = db.createCollection(&session, csOptions.logicalID, "bar1", 2, clOptions);
   ASSERT_EQ(SDB_DMS_EXIST, rc);
   rc = db.createCollection(&session, csOptions.logicalID, "bar2", 1, clOptions);
   ASSERT_EQ(SDB_DMS_EXIST, rc);
   rc = db.createCollection(&session, csOptions.logicalID, "bar2", 2, clOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.listCollections(&session, csOptions.logicalID, NULL, &cursor);
   ASSERT_EQ(SDB_OK, rc);

   rc = cursor.getNext(&session, slice);
   ASSERT_EQ(SDB_OK, rc);
   record = (const listCollectionsRecord *)(slice.data());
   ASSERT_EQ(0, record->mbID);
   ASSERT_EQ(1, record->logicalID);
   ASSERT_EQ(0, ossStrcmp("bar1", record->name));

   rc = cursor.getNext(&session, slice);
   ASSERT_EQ(SDB_OK, rc);
   record = (const listCollectionsRecord *)(slice.data());
   ASSERT_EQ(1, record->mbID);
   ASSERT_EQ(2, record->logicalID);
   ASSERT_EQ(0, ossStrcmp("bar2", record->name));

   rc = cursor.getNext(&session, slice);
   ASSERT_EQ(SDB_VESSEL_END_OF_CURSOR, rc);

   db.close(&session, closeDBOptions());
}


TEST_F(vesseltest, test6)
{
   INT32 rc = SDB_OK;
   test_logger logger;
   vesselImpl db;
   outerResource resource;
   resource.logger = &logger; 
   test_session session;
   openDBOptions options;
   createCSOptions csOptions;
   createCLOptions clOptions;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobMetaPath = DATA_PATH;
   options.path.lobPath = DATA_PATH;

   UINT32 count = 0;

   db.setup(resource);
   rc = db.open(&session, options);
   ASSERT_EQ(SDB_OK, rc);

   csOptions.logicalID = 1;
   rc = db.createCollectionSpace(&session, "foo", csOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollection(&session, csOptions.logicalID, "bar1", 1, clOptions);
   ASSERT_EQ(SDB_OK, rc);
   rc = db.createCollection(&session, csOptions.logicalID, "bar2", 2, clOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.open(&session, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollection(&session, csOptions.logicalID, "bar1", 1, clOptions);
   ASSERT_EQ(SDB_DMS_EXIST, rc);
   rc = db.createCollection(&session, csOptions.logicalID, "bar2", 2, clOptions);
   ASSERT_EQ(SDB_DMS_EXIST, rc);
   rc = db.createCollection(&session, csOptions.logicalID, "bar3", 3, clOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);

    rc = db.open(&session, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollection(&session, csOptions.logicalID, "bar1", 1, clOptions);
   ASSERT_EQ(SDB_DMS_EXIST, rc);
   rc = db.createCollection(&session, csOptions.logicalID, "bar2", 2, clOptions);
   ASSERT_EQ(SDB_DMS_EXIST, rc);
   rc = db.createCollection(&session, csOptions.logicalID, "bar3", 3, clOptions);
   ASSERT_EQ(SDB_DMS_EXIST, rc);
   rc = db.createCollection(&session, csOptions.logicalID, "bar4", 4, clOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}