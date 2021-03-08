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

   Source File Name = cursor_test.cpp

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

#include "vessel/cursorKernal.h"
#include "vessel/ISession.h"
#include "vessel/vesselDef.h"
#include "vessel/vesselImpl.h"

#include "gtest/gtest.h"

using namespace engine::vessel;

class test_session : public ISession
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

class test_cursor : public cursorKernal
{
   public:
      test_cursor(){}
      virtual ~test_cursor(){}

   public:
      virtual CURSOR_TYPE getType()const
      {
         return CURSOR_TYPE_INVALID;
      }
};

class test_vessel : public vesselImpl
{
   public:
         virtual INT32 pushMoreToCursor(ISession * session,
                                        cursorKernal *cursor)
         {
            cursor->pushEnd();
            return SDB_OK;
         }
};


TEST(cursortest, tes1)
{
   cursorOptions options;
   CHAR buf[1020] = {0};
   slice src;
   src.reset(1020, buf);
   slice content;
   INT32 rc = SDB_OK;
   test_vessel db;
   test_session session;
   test_cursor cursor;
   rc = cursor.open(&db, NULL, NULL);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < 32; ++i)
   {
      rc = cursor.push(src);
      ASSERT_EQ(SDB_OK, rc);
   }

   rc = cursor.push(src);
   ASSERT_EQ(SDB_VESSEL_CURSOR_NO_SPACE, rc);

   for (UINT32 i = 0; i < 32; ++i)
   {
      rc = cursor.getNext(&session, content);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(1020, content.len());
   }
   rc = cursor.getNext(&session, content);
   ASSERT_EQ(SDB_VESSEL_END_OF_CURSOR, rc);

   rc = cursor.close();
   ASSERT_EQ(SDB_OK, rc);
}

TEST(cursortest, tes2)
{
   cursorOptions options;
   options.initBufSize = 1024;
   CHAR buf[2048] = {0};
   slice src;
   src.reset(2048, buf);
   slice content;
   INT32 rc = SDB_OK;
   test_vessel db;
   test_session session;
   test_cursor cursor;
   rc = cursor.open(&db, NULL, &options);
   ASSERT_EQ(SDB_OK, rc);

   rc = cursor.push(src);
   ASSERT_EQ(SDB_OK, rc);

   rc = cursor.push(src);
   ASSERT_EQ(SDB_VESSEL_CURSOR_NO_SPACE, rc);

   rc = cursor.getNext(&session, content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(2048, content.len());

   rc = cursor.getNext(&session, content);
   ASSERT_EQ(SDB_VESSEL_END_OF_CURSOR, rc);

   rc = cursor.close();
   ASSERT_EQ(SDB_OK, rc);
}

TEST(cursortest, test3)
{
   cursorOptions options;
   options.initBufSize = 1024;
   options.maxBufSize = 1024;
   CHAR buf[2048] = {0};
   slice src;
   src.reset(2048, buf);
   slice content;
   INT32 rc = SDB_OK;
   test_vessel db;
   test_session session;
   test_cursor cursor;
   rc = cursor.open(&db, NULL, &options);
   ASSERT_EQ(SDB_OK, rc);

   rc = cursor.push(src);
   ASSERT_EQ(SDB_VESSEL_OUT_OF_RESOURCE, rc);

   rc = cursor.close();
   ASSERT_EQ(SDB_OK, rc);
}