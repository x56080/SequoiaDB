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
#include "vessel/vesselImpl.h"
#include "test_def.h"

#include "gtest/gtest.h"

using namespace engine::vessel;

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
	    test_vessel(){}
		virtual ~test_vessel(){}
         virtual INT32 pushMoreToCursor(engine::vessel::ISession * session,
                                        cursorKernal *cursor)
         {
            cursor->pushEnd();
            return SDB_OK;
         }
};


TEST(cursortest, test1)
{
   cursorOptions options;
   CHAR buf[1020] = {0};
   slice src;
   src.reset(1020, buf);
   slice content;
   INT32 rc = SDB_OK;
   test_vessel db;
   test_logger logger;
   test_session session(&logger);
   test_cursor cursor;
   rc = cursor.open(&db, NULL, options);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < 64; ++i)
   {
      rc = cursor.push(src);
      ASSERT_EQ(SDB_OK, rc);
   }

   rc = cursor.push(src);
   ASSERT_EQ(SDB_VESSEL_CURSOR_NO_SPACE, rc);

   for (UINT32 i = 0; i < 64; ++i)
   {
      rc = cursor.getNext(&session, content);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(1020, content.len());
   }
   rc = cursor.getNext(&session, content);
   ASSERT_EQ(SDB_VESSEL_EOC, rc);

   cursor.close();
}

TEST(cursortest, test2)
{
   cursorOptions options;
   options.initBufSize = 1024;
   CHAR buf[2048] = {0};
   slice src;
   src.reset(2048, buf);
   slice content;
   INT32 rc = SDB_OK;
   test_vessel db;
   test_logger logger;
   test_session session(&logger);
   test_cursor cursor;
   rc = cursor.open(&db, NULL, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = cursor.push(src);
   ASSERT_EQ(SDB_OK, rc);

   rc = cursor.push(src);
   ASSERT_EQ(SDB_VESSEL_CURSOR_NO_SPACE, rc);

   rc = cursor.getNext(&session, content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(2048, content.len());

   rc = cursor.getNext(&session, content);
   ASSERT_EQ(SDB_VESSEL_EOC, rc);

   cursor.close();
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
   test_logger logger;
   test_session session(&logger);
   test_cursor cursor;
   rc = cursor.open(&db, NULL, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = cursor.push(src);
   ASSERT_EQ(SDB_VESSEL_OUT_OF_RESOURCE, rc);

   cursor.close();
}