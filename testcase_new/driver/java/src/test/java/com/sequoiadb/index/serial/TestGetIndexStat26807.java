package com.sequoiadb.index.serial;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;
import java.util.ArrayList;
import java.util.List;

/**
 * @descreption seqDB-26807:使用getIndexStat获取索引统计信息指定detail
 * @author Huanghaimei
 * @date 2022/8/11
 * @updateUser
 * @updateDate
 * @updateRemark
 * @version 1.0
 */
public class TestGetIndexStat26807 extends SdbTestBase {
    private Sequoiadb sdb;
    private String csName = "cs_26807";
    private String clName = "cl_26807";
    private String idxName = "idx_26807";
    private DBCollection cl;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace collectionSpace = sdb.createCollectionSpace( csName );
        cl = collectionSpace.createCollection( clName );
        List< BSONObject > list = new ArrayList<>();
        BSONObject obj = new BasicBSONObject();
        obj.put( "a", 1 );
        list.add( obj );
        cl.bulkInsert( list );
        cl.createIndex( idxName, new BasicBSONObject( "a", 1 ),
                new BasicBSONObject( "NotNull", false ) );
    }

    @Test
    public void test() {
        BasicBSONObject options = new BasicBSONObject();
        options.put( "Collection", csName + "." + clName );
        sdb.analyze( options );
        BSONObject indexStatTrue = cl.getIndexStat( idxName, true );
        BSONObject indexStatFalse = cl.getIndexStat( idxName, false );
        checkIndexStat( indexStatTrue, true );
        checkIndexStat( indexStatFalse, false );
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.dropCollectionSpace( csName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void checkIndexStat( BSONObject indexStat, boolean getDetail ) {
        Assert.assertEquals( indexStat.get( "Collection" ), cl.getFullName() );
        Assert.assertEquals( indexStat.get( "Index" ), idxName );
        Assert.assertEquals( indexStat.get( "Unique" ), false );
        Assert.assertEquals( indexStat.get( "KeyPattern" ).toString(),
                "{ \"a\" : 1 }" );
        Assert.assertNotEquals( indexStat.get( "TotalIndexLevels" ), 0 );
        Assert.assertNotEquals( indexStat.get( "TotalIndexPages" ), 0 );
        Assert.assertEquals( indexStat.get( "DistinctValNum" ).toString(),
                "[" + 1 + "]" );
        Assert.assertEquals( indexStat.get( "MinValue" ).toString(),
                "{ \"a\" : 1 }" );
        Assert.assertEquals( indexStat.get( "MaxValue" ).toString(),
                "{ \"a\" : 1 }" );
        Assert.assertEquals( indexStat.get( "NullFrac" ), 0 );
        Assert.assertEquals( indexStat.get( "UndefFrac" ), 0 );
        if ( getDetail ) {
            BSONObject mcv = ( BSONObject ) indexStat.get( "MCV" );
            BasicBSONList listValues = ( BasicBSONList ) mcv.get( "Values" );
            BasicBSONList listFrac = ( BasicBSONList ) mcv.get( "Frac" );
            Assert.assertEquals( listValues.get( 0 ).toString(),
                    "{ \"a\" : 1 }" );
            Assert.assertEquals( listFrac.get( 0 ), 10000 );
        }
        Assert.assertEquals( indexStat.get( "SampleRecords" ),
                Long.parseLong( 1 + "" ) );
        Assert.assertEquals( indexStat.get( "TotalRecords" ),
                Long.parseLong( 1 + "" ) );
        Assert.assertNotNull( indexStat.get( "StatTimestamp" ) );
    }
}