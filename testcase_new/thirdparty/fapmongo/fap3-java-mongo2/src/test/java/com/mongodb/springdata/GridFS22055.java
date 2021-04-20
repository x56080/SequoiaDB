package com.mongodb.springdata;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.net.UnknownHostException;
import java.util.Arrays;
import java.util.List;
import java.util.Random;
import java.util.Set;

import org.springframework.data.mongodb.core.query.Query;
import org.springframework.data.mongodb.gridfs.GridFsTemplate;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.DBCollection;
import com.mongodb.DBObject;
import com.mongodb.util.JSON;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description: seqDB-22055:存在gridfs的集合下，列取索引/列取集合/删除集合
 * @author fanyu
 * @Date 2020/4/9
 * @version 1.00
 */
public class GridFS22055 extends MongodbTestBase {
    private GridFsTemplate gridFsTemplate;
    private String bucketName = springDBNameWithVersion + "_bucket22055";
    private String[] expClNames = { bucketName + "." + "chunks",
            bucketName + "." + "files" };

    @BeforeClass
    public void setUp() throws UnknownHostException {
        gridFsTemplate = new GridFsTemplate( mongoDbFactory, converter,
                bucketName );
    }

    @Test
    public void test1() throws IOException {
        // 集合存在文件，使用查询条件删除文件
        // 创建文件，使gridFS对应的集合存在
        byte[] bytes = new byte[ 1024 ];
        new Random().nextBytes( bytes );
        String fileName = "fs22055";
        gridFsTemplate.store( new ByteArrayInputStream( bytes ), fileName );

        // 列取集合名
        Set< String > clNames = mongoTemplate.getCollectionNames();
        Assert.assertTrue( clNames.size() >= expClNames.length );
        Assert.assertTrue( clNames.containsAll( Arrays.asList( expClNames ) ),
                "clNames = " + clNames.toString() );

        // 列取索引
        DBCollection cl1 = mongoTemplate.getCollection( expClNames[ 0 ] );
        List< DBObject > indexInfo1 = cl1.getIndexInfo();
        Assert.assertEquals( indexInfo1.size(), 1, indexInfo1.toString() );
        Assert.assertEquals( indexInfo1.get( 0 ).get( "key" ),
                JSON.parse( "{ \"_id\" : 1}" ) );
        Assert.assertEquals( indexInfo1.get( 0 ).get( "ns" ),
                mongoTemplate.getDb().getName() + "." + expClNames[ 0 ] );

        DBCollection cl2 = mongoTemplate.getCollection( expClNames[ 1 ] );
        List< DBObject > indexInfo2 = cl2.getIndexInfo();
        Assert.assertEquals( indexInfo2.size(), 1, indexInfo1.toString() );
        Assert.assertEquals( indexInfo2.get( 0 ).get( "key" ),
                JSON.parse( "{ \"_id\" : 1}" ) );
        Assert.assertEquals( indexInfo2.get( 0 ).get( "ns" ),
                mongoTemplate.getDb().getName() + "." + expClNames[ 1 ] );

        // 删除索引 sequoiadb只有一个id索引，mongodb会有两个索引
        // cl1.dropIndex( "files_id_1_n_1" );
        // cl2.dropIndex( "filename_1_uploadDate_1" );
        Assert.assertEquals( cl1.getIndexInfo().size(), 1 );
        Assert.assertEquals( cl2.getIndexInfo().size(), 1 );

        // count集合
        Assert.assertEquals(
                mongoTemplate.count( new Query(), expClNames[ 0 ] ), 1 );
        Assert.assertEquals(
                mongoTemplate.count( new Query(), expClNames[ 1 ] ), 1 );
        gridFsTemplate.delete( new Query() );
        Assert.assertEquals(
                mongoTemplate.count( new Query(), expClNames[ 0 ] ), 0 );
        Assert.assertEquals(
                mongoTemplate.count( new Query(), expClNames[ 1 ] ), 0 );

        // 删除集合名
        for ( String clName : expClNames ) {
            mongoTemplate.getCollection( clName ).drop();
        }
        Set< String > clNames1 = mongoTemplate.getCollectionNames();
        Assert.assertFalse( clNames1.containsAll( Arrays.asList( expClNames ) ),
                "clNames1 = " + clNames1.toString() );
    }

    @AfterClass
    public void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), mongoTemplate,
                expClNames );
    }
}
