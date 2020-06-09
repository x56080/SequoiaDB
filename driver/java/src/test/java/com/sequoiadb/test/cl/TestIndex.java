package com.sequoiadb.test.cl;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.common.Constants;
import com.sequoiadb.test.common.ConstantsInsert;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

import java.util.List;

import static org.junit.Assert.assertTrue;

public class TestIndex {
    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;
    private String idxName = "haha";

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {

    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {

    }

    @Before
    public void setUp() throws Exception {
        // sdb
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        // cs
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
        }

        cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        BSONObject conf = new BasicBSONObject();
        conf.put("ReplSize", 0);
        cl = cs.createCollection(Constants.TEST_CL_NAME_1, conf);
        List<BSONObject> list = ConstantsInsert.createRecordList(100);
        cl.bulkInsert(list, DBCollection.FLG_INSERT_CONTONDUP);
    }

    @After
    public void tearDown() throws Exception {
        sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
        sdb.disconnect();
    }

    @Test
    public void testCreateIndex() {
        BasicBSONObject key = new BasicBSONObject("a", 1);
        cl.createIndex(idxName, key, false, false);

        DBCursor cursor = cl.getIndex(idxName);
        assertTrue(cursor.hasNext());
        cl.dropIndex(idxName);
    }

    @Test
    public void testGetEmptyIndex() {

        String emptyIndexName = "aaaaaaaaa";
        // case 1:
        DBCursor cursor;
        cursor = cl.getIndex(emptyIndexName);
        while(cursor.hasNext()) {
            System.out.println("index is: " + cursor.getNext());
        }

        // case 2:
        Assert.assertFalse(cl.isIndexExist(emptyIndexName));
        try {
            cl.getIndexInfo(emptyIndexName);
            Assert.fail();
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_IXM_NOTEXIST.getErrorCode(),
                    e.getErrorCode());
        }

        // case 3:
        String idIdxName = "$id";
        Assert.assertTrue(cl.isIndexExist(idIdxName));
        BSONObject indexObj = cl.getIndexInfo(idIdxName);
        Assert.assertNotNull(indexObj);
        System.out.println("id index is: " + indexObj.toString());

    }

    @Test
    public void testCreateIndexWithOptions(){
        BasicBSONObject key = new BasicBSONObject();
        String name = "name";
        String name2 = "name2";
        String name3 = "name3";
        String name4 = "name4";
        key.put(name,1);
        BSONObject indexObj;

        BasicBSONObject optionsCase1 = new BasicBSONObject();
        cl.createIndex(name,key,optionsCase1);
        indexObj = cl.getIndexInfo(name);
        Assert.assertNotNull(indexObj);
        System.out.println("Case1 index is: " + indexObj.toString());
        cl.dropIndex(name);

        BasicBSONObject optionsCase2 = null;
        cl.createIndex(name2,key,optionsCase2);
        indexObj = cl.getIndexInfo(name2);
        Assert.assertNotNull(indexObj);
        System.out.println("Case2 index is: " + indexObj.toString());
        cl.dropIndex(name2);

        BasicBSONObject optionsCase3 = new BasicBSONObject();
        optionsCase3.put("Unique",true);
        optionsCase3.put("Enforced",true);
        optionsCase3.put("NotNull",false);
        optionsCase3.put("SortBufferSize", 100);
        cl.createIndex(name3, new BasicBSONObject().append("Id", 1),optionsCase3);
        indexObj = cl.getIndexInfo(name3);
        Assert.assertNotNull(indexObj);
        System.out.println("Case3 index is: " + indexObj.toString());
        cl.dropIndex(name3);

        BasicBSONObject optionsCase4 = new BasicBSONObject();
        optionsCase4.put("Unique",true);
        optionsCase4.put("Enforced",false);
        optionsCase4.put("NotNull",false);
        optionsCase4.put("SortBufferSize", 64);
        cl.createIndex(name4,key,optionsCase4);
        indexObj = cl.getIndexInfo(name4);
        Assert.assertNotNull(indexObj);
        System.out.println("Case4 index is: " + indexObj.toString());
        cl.dropIndex(name4);
    }
}