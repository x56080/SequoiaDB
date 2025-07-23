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

   Source File Name = SdbChangeStreamTest.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.test.changestream;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.stream.Collectors;

import static org.junit.Assert.*;


public class SdbChangeStreamTest {

    private final BSONObject emptyBson = new BasicBSONObject();

    private final String csName1 = Constants.TEST_CS_NAME_1;
    private final String csName2 = Constants.TEST_CS_NAME_2;
    private final String clName1 = Constants.TEST_CL_NAME_1;
    private final String clName2 = Constants.TEST_CL_NAME_2;

    private final BSONObject insertData = new BasicBSONObject("name", "test");

    private Sequoiadb coordSdb;
    private Sequoiadb dataSdb;

    @Before
    public void setUp() {
        coordSdb = new Sequoiadb(Constants.HOST, Constants.PORT, Constants.TEST_USER_NAME,
                Constants.TEST_USER_PASSWORD);
        dataSdb = new Sequoiadb(Constants.DATA_HOST, Constants.DATA_PORT, Constants.TEST_USER_NAME,
                Constants.TEST_USER_PASSWORD);

        CollectionSpace cs1 = createNewCS(coordSdb, csName1);
        CollectionSpace cs2 = createNewCS(coordSdb, csName2);
        createNewCL(cs1, clName1, Constants.GROUPNAME);
        createNewCL(cs1, clName2, Constants.GROUPNAME);
        createNewCL(cs2, clName1, Constants.GROUPNAME);
    }

    @After
    public void tearDown() {
        dropCSIfExists(coordSdb, csName1);
        dropCSIfExists(coordSdb, csName2);
        dataSdb.close();
        coordSdb.close();
    }

    @Test
    public void getListTest() {
        // no watch
        try (DBCursor cursor = coordSdb.getList(Sequoiadb.SDB_LIST_STREAMS, emptyBson, emptyBson, emptyBson)) {
            assertFalse(cursor.hasNext());
        }

        // two watch
        BSONObject options1 = new BasicBSONObject("CollectionSpaces", Collections.singletonList(csName1));
        BSONObject options2 = new BasicBSONObject("CollectionSpaces", Collections.singletonList(csName2));
        try (DBCursor ignored1 = dataSdb.watch(new StreamToken(), options1, emptyBson);
             DBCursor ignored2 = dataSdb.watch(new StreamToken(), options2, emptyBson);
             DBCursor cursor = coordSdb.getList(Sequoiadb.SDB_LIST_STREAMS, emptyBson, emptyBson, emptyBson)) {

            List<BSONObject> res = new ArrayList<>();
            while (cursor.hasNext()) {
                res.add(cursor.getNext());
            }

            List<Object> collectionSpaces = res.stream()
                    .map(v -> v.get("Options"))
                    .filter(v -> v instanceof BSONObject)
                    .map(v -> ((BSONObject) v).get("CollectionSpaces"))
                    .collect(Collectors.toList());

            assertEquals(2, collectionSpaces.size());
            assertTrue(collectionSpaces.contains(Collections.singletonList(csName1)));
            assertTrue(collectionSpaces.contains(Collections.singletonList(csName2)));
        }
    }

    @Test
    public void getSnapshotTest() {
        // no watch
        try (DBCursor cursor = coordSdb.getSnapshot(Sequoiadb.SDB_SNAP_STREAMS, emptyBson, emptyBson, emptyBson)) {
            assertFalse(cursor.hasNext());
        }

        // two watch
        BSONObject options1 = new BasicBSONObject("CollectionSpaces", Collections.singletonList(csName1));
        BSONObject options2 = new BasicBSONObject("CollectionSpaces", Collections.singletonList(csName2));
        try (DBCursor ignored1 = dataSdb.watch(new StreamToken(), options1, emptyBson);
             DBCursor ignored2 = dataSdb.watch(new StreamToken(), options2, emptyBson);
             DBCursor cursor = coordSdb.getSnapshot(Sequoiadb.SDB_SNAP_STREAMS, emptyBson, emptyBson, emptyBson)) {
            List<BSONObject> res = new ArrayList<>();
            while (cursor.hasNext()) {
                res.add(cursor.getNext());
            }

            List<Object> collectionSpaces = res.stream()
                    .map(v -> v.get("Options"))
                    .filter(v -> v instanceof BSONObject)
                    .map(v -> ((BSONObject) v).get("CollectionSpaces"))
                    .collect(Collectors.toList());

            assertEquals(2, collectionSpaces.size());
            assertTrue(collectionSpaces.contains(Collections.singletonList(csName1)));
            assertTrue(collectionSpaces.contains(Collections.singletonList(csName2)));
        }
    }

    @Test
    public void getChangeStreamTokenTest() {
        StreamToken changeStreamToken = dataSdb.getChangeStreamToken();

        DBCollection collection = coordSdb
                .getCollectionSpace(csName1)
                .getCollection(clName1);

        collection.insertRecord(insertData);

        BSONObject options = new BasicBSONObject("Collections", Collections.singletonList(csName1 + "." + clName1));
        try (DBCursor watch = dataSdb.watch(changeStreamToken, options, emptyBson)) {
            BSONObject next = watch.getNext();
            assertEqualsRes(csName1, clName1, next);

            assertEquals(next.get("Token"), dataSdb.getChangeStreamToken().getToken());
        }
    }

    @Test
    public void dbWatchTest() {
        DBCollection collection1 = coordSdb.getCollectionSpace(csName1).getCollection(clName1);
        DBCollection collection2 = coordSdb.getCollectionSpace(csName1).getCollection(clName2);
        DBCollection collection3 = coordSdb.getCollectionSpace(csName2).getCollection(clName1);

        // watch two cs
        ArrayList<String> list = new ArrayList<>();
        Collections.addAll(list, csName1, csName2);
        BSONObject options = new BasicBSONObject("CollectionSpaces", list);
        try (DBCursor watch = dataSdb.watch(new StreamToken(), options, emptyBson)) {
            collection1.insertRecord(insertData);
            collection2.insertRecord(insertData);
            collection3.insertRecord(insertData);

            BSONObject next = watch.getNext();
            assertEqualsRes(csName1, clName1, next);

            next = watch.getNext();
            assertEqualsRes(csName1, clName2, next);

            next = watch.getNext();
            assertEqualsRes(csName2, clName1, next);
        }

        // watch one cs
        list.clear();
        Collections.addAll(list, csName1);
        options = new BasicBSONObject("CollectionSpaces", Collections.singletonList(csName1));
        try (DBCursor watch = dataSdb.watch(new StreamToken(), options, emptyBson)) {
            collection1.insertRecord(insertData);
            collection2.insertRecord(insertData);
            collection3.insertRecord(insertData);

            BSONObject next = watch.getNext();
            assertEqualsRes(csName1, clName1, next);

            next = watch.getNext();
            assertEqualsRes(csName1, clName2, next);
        }

        // watch two cl
        list.clear();
        Collections.addAll(list, collection1.getFullName(), collection2.getFullName());
        options = new BasicBSONObject("Collections", list);
        try (DBCursor watch = dataSdb.watch(new StreamToken(), options, emptyBson)) {
            collection1.insertRecord(insertData);
            collection2.insertRecord(insertData);
            collection3.insertRecord(insertData);

            BSONObject next = watch.getNext();
            assertEqualsRes(csName1, clName1, next);

            next = watch.getNext();
            assertEqualsRes(csName1, clName2, next);
        }

        // watch one cl
        list.clear();
        Collections.addAll(list, collection2.getFullName());
        options = new BasicBSONObject("Collections", list);
        try (DBCursor watch = dataSdb.watch(new StreamToken(), options, emptyBson)) {
            collection1.insertRecord(insertData);
            collection2.insertRecord(insertData);
            collection3.insertRecord(insertData);

            BSONObject next = watch.getNext();
            assertEqualsRes(csName1, clName2, next);
        }
    }

    @Test
    public void csWatchTest() {
        DBCollection collection1 = coordSdb.getCollectionSpace(csName1).getCollection(clName1);
        DBCollection collection2 = coordSdb.getCollectionSpace(csName1).getCollection(clName2);

        // Invalid options
        BSONObject options = new BasicBSONObject("CollectionSpaces", Collections.singletonList(csName2));
        options.put("Collections", Collections.singletonList(collection2.getFullName()));

        try (DBCursor watch = dataSdb.getCollectionSpace(csName1).watch(new StreamToken(), options, emptyBson)) {
            collection1.insertRecord(insertData);
            collection2.insertRecord(insertData);

            BSONObject next = watch.getNext();
            assertEqualsRes(csName1, clName1, next);

            next = watch.getNext();
            assertEqualsRes(csName1, clName2, next);
        }
    }

    @Test
    public void clWatchTest() {
        DBCollection collection1 = coordSdb.getCollectionSpace(csName1).getCollection(clName1);
        DBCollection collection2 = coordSdb.getCollectionSpace(csName1).getCollection(clName2);

        // Invalid options
        BSONObject options = new BasicBSONObject("CollectionSpaces", Collections.singletonList(csName2));
        options.put("Collections", Collections.singletonList(collection2.getFullName()));

        try (DBCursor watch = dataSdb.getCollectionSpace(csName1)
                .getCollection(clName1)
                .watch(new StreamToken(), options, emptyBson)) {
            collection1.insertRecord(insertData);

            BSONObject next = watch.getNext();
            assertEqualsRes(csName1, clName1, next);
        }
    }

    private CollectionSpace createNewCS(Sequoiadb sequoiadb, String csName) {
        try {
            return sequoiadb.createCollectionSpace(csName, emptyBson);
        } catch (BaseException e) {
            if (e.getErrorCode() == SDBError.SDB_DMS_CS_EXIST.getErrorCode()) {
                sequoiadb.dropCollectionSpace(csName);
                return sequoiadb.createCollectionSpace(csName, emptyBson);
            }
            throw e;
        }
    }

    private void createNewCL(CollectionSpace cs, String clName, String groupName) {
        BasicBSONObject options = new BasicBSONObject();
        options.put("Group", groupName);
        try {
            cs.createCollection(clName, options);
        } catch (BaseException e) {
            if (e.getErrorCode() == SDBError.SDB_DMS_EXIST.getErrorCode()) {
                cs.dropCollection(clName);
                cs.createCollection(clName, options);
                return;
            }
            throw e;
        }
    }

    private void dropCSIfExists(Sequoiadb sequoiadb, String csName) {
        try {
            sequoiadb.dropCollectionSpace(csName);
        } catch (BaseException e) {
            if (e.getErrorCode() != SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode()) {
                throw e;
            }
        }
    }

    private void assertEqualsRes(String csName, String clName, BSONObject res) {
        assertEquals("change", res.get("Type"));
        assertEquals("insert", res.get("ChangeType"));
        assertEquals(csName, res.get("CollectionSpace"));
        assertEquals(csName + "." + clName, res.get("Collection"));
        assertTrue(res.containsField("DocumentKey"));

        BSONObject doc = (BSONObject) res.get("Document");
        assertEquals("test", doc.get("name"));
    }

}
