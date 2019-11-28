package com.sequoiadb.crud;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.util.JSON;
import org.bson.util.JSONParseException;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @FileName:seqDB-6608:insert wrong format BinData
 * @Author linsuqiang
 * @Date 2016-12-22
 * @Version 1.00
 */

public class BinFormatTest6608 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private final String clName = "cl6608";

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cs = sdb.getCollectionSpace( SdbTestBase.csName );
            cl = cs.createCollection( clName );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    @Test
    public void test() {
        try {
            BSONObject doc2 = ( BSONObject ) JSON.parse(
                    "{ a: { '$binary': 'aGVsbG8gd29ybGQ', '$type': '1' } } " );
            cl.insert( doc2 );
            Assert.fail( doc2 + " should not be inserted" );
        } catch ( JSONParseException e ) {
        }

        String doc3 = "{ a: { '$binary': 'aGVsbG8gd29ybGQ', '$type': '1' } } ";
        try {
            cl.insert( doc3 );
            Assert.fail( doc3 + " should not be inserted" );
        } catch ( JSONParseException e ) {
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            cs.dropCollection( clName );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -23, e.getMessage() );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }
}
