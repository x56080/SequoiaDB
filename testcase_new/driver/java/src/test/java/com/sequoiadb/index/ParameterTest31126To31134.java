package com.sequoiadb.index;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Descreption seqDB-31126:createIndexcreateIndex
 *              (<name>,<indexDef>,[indexAttr], [option] )接口参数校验
 *              seqDB-31127:cl.createIndexAsync接口参数校验
 *              seqDB-31128:dropIndexAsync接口参数校验 seqDB-31132:copyIndex接口参数校验
 *              seqDB-31134:copyIndexAsync接口参数校验
 * @Author Cheng Jingjing
 * @CreateDate 2023/4/17
 * @UpdateUser Cheng Jingjing
 * @UpdateDate 2023/4/17
 * @UpdateRemark
 * @Version 1.0
 */
public class ParameterTest31126To31134 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( csName );
    }

    @Test
    public void test31126() {
        String clName = "cl_31126";
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        cl = cs.createCollection( clName );
        // case1: indexName为""
        try {
            cl.createIndex( "", new BasicBSONObject( "a", 1 ),
                    new BasicBSONObject( "Enforced", false ),
                    new BasicBSONObject( "SortBufferSize", 2048 ) );
            Assert.fail( "expect fail but success." );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -6 ) {
                cs.dropCollection( clName );
                throw e;
            }
        }

        // case2: indexKeys为null
        try {
            cl.createIndex( "idx_31126", null,
                    new BasicBSONObject( "Enforced", false ),
                    new BasicBSONObject( "SortBufferSize", 2048 ) );
            Assert.fail( "expect fail but success." );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -6 ) {
                cs.dropCollection( clName );
                throw e;
            }
        }

        // case3: indexAttr的参数值不为boolean
        try {
            cl.createIndex( "idx_31126", new BasicBSONObject( "a", 1 ),
                    new BasicBSONObject( "Enforced", "true" ),
                    new BasicBSONObject( "SortBufferSize", 2048 ) );
            Assert.fail( "expect fail but success." );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -6 ) {
                cs.dropCollection( clName );
                throw e;
            }
        }

        // case4: SortBufferSize的参数值超过规定范围
        try {
            cl.createIndex( "idx_31126", new BasicBSONObject( "a", 1 ),
                    new BasicBSONObject( "Enforced", false ),
                    new BasicBSONObject( "SortBufferSize", -1 ) );
            Assert.fail( "expect fail but success." );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -6 ) {
                cs.dropCollection( clName );
                throw e;
            }
        }

        // case5： 正常创建
        cl.createIndexAsync( "idx_31126", new BasicBSONObject( "a", 1 ),
                new BasicBSONObject( "Enforced", false ),
                new BasicBSONObject( "SortBufferSize", 2048 ) );
        cl.insertRecord( new BasicBSONObject( "a", 1 ) );
        cs.dropCollection( clName );
    }

    @Test
    public void test31127() {
        String clName = "cl_31127";
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        cl = cs.createCollection( clName );
        // case1: indexName为""
        try {
            cl.createIndexAsync( "", new BasicBSONObject( "a", 1 ),
                    new BasicBSONObject( "Enforced", false ),
                    new BasicBSONObject( "SortBufferSize", 2048 ) );
            Assert.fail( "expect fail but success." );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -6 ) {
                cs.dropCollection( clName );
                throw e;
            }
        }

        // case2: indexKeys为null
        try {
            cl.createIndexAsync( "idx_31127", null,
                    new BasicBSONObject( "Enforced", false ),
                    new BasicBSONObject( "SortBufferSize", 2048 ) );
            Assert.fail( "expect fail but success." );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -6 ) {
                cs.dropCollection( clName );
                throw e;
            }
        }

        // case3: indexAttr的参数值不为boolean
        try {
            cl.createIndexAsync( "idx_31127", new BasicBSONObject( "a", 1 ),
                    new BasicBSONObject( "Enforced", "true" ),
                    new BasicBSONObject( "SortBufferSize", 2048 ) );
            Assert.fail( "expect fail but success." );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -6 ) {
                cs.dropCollection( clName );
                throw e;
            }
        }

        // case4: SortBufferSize的参数值超过规定范围
        try {
            cl.createIndexAsync( "idx_31127", new BasicBSONObject( "a", 1 ),
                    new BasicBSONObject( "Enforced", false ),
                    new BasicBSONObject( "SortBufferSize", -1 ) );
            Assert.fail( "expect fail but success." );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -6 ) {
                cs.dropCollection( clName );
                throw e;
            }
        }

        // case5： 正常创建
        cl.createIndexAsync( "idx_31127", new BasicBSONObject( "a", 1 ),
                new BasicBSONObject( "Enforced", false ),
                new BasicBSONObject( "SortBufferSize", 2048 ) );
        cl.insertRecord( new BasicBSONObject( "a", 1 ) );
        cs.dropCollection( clName );
    }

    @Test
    public void test31128() {
        String clName = "cl_31128";
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        cl = cs.createCollection( clName );
        cl.createIndexAsync( "idx_31128", new BasicBSONObject( "a", 1 ),
                new BasicBSONObject( "Enforced", false ),
                new BasicBSONObject( "SortBufferSize", 2048 ) );
        cl.insertRecord( new BasicBSONObject( "a", 1 ) );

        // case1: indexName为""
        try {
            cl.dropIndexAsync( "" );
            Assert.fail( "expect fail but success." );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -6 ) {
                cs.dropCollection( clName );
                throw e;
            }
        }

        // case2: 正常删除
        cl.dropIndexAsync( "idx_31128" );
        cs.dropCollection( clName );
    }

    @Test
    public void test31132() {
        String mainClName = "cl_31132_main";
        String subCLName1 = "cl_31132_1";
        if ( cs.isCollectionExist( mainClName ) ) {
            cs.dropCollection( mainClName );
        }
        // 创建主子表并插入数据
        BasicBSONObject option = new BasicBSONObject();
        option.put( "IsMainCL", true );
        option.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        option.put( "ShardingType", "range" );
        DBCollection mainCL = cs.createCollection( mainClName, option );

        BasicBSONObject subOption = new BasicBSONObject();
        subOption.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        subOption.put( "ShardingType", "hash" );
        cs.createCollection( subCLName1, subOption );

        BasicBSONObject subCLBound = new BasicBSONObject();
        subCLBound.put( "LowBound", new BasicBSONObject( "a", 0 ) );
        subCLBound.put( "UpBound", new BasicBSONObject( "a", 200 ) );
        mainCL.attachCollection( csName + "." + subCLName1, subCLBound );

        int recsNum = 100;
        List< BSONObject > insertRecords = new ArrayList<>();
        for ( int i = 0; i < recsNum; i++ ) {
            BSONObject obj = new BasicBSONObject();
            obj.put( "a", i );
            obj.put( "b", i );
            insertRecords.add( obj );
        }
        mainCL.bulkInsert( insertRecords );
        mainCL.createIndex( "idx_31132", new BasicBSONObject( "b", 1 ),
                new BasicBSONObject( "Enforced", false ),
                new BasicBSONObject( "SortBufferSize", 2048 ) );

        // case1: subClName为""
        mainCL.copyIndex( "", "idx_31132" );

        // case2: indexName为""
        mainCL.copyIndex( csName + "." + subCLName1, "" );

        cs.dropCollection( mainClName );
    }

    @Test
    public void test31134() {
        String mainClName = "cl_31134_main";
        String subCLName1 = "cl_31134_1";
        if ( cs.isCollectionExist( mainClName ) ) {
            cs.dropCollection( mainClName );
        }
        // 创建主子表并插入数据
        BasicBSONObject option = new BasicBSONObject();
        option.put( "IsMainCL", true );
        option.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        option.put( "ShardingType", "range" );
        DBCollection mainCL = cs.createCollection( mainClName, option );

        BasicBSONObject subOption = new BasicBSONObject();
        subOption.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        subOption.put( "ShardingType", "hash" );
        cs.createCollection( subCLName1, subOption );

        BasicBSONObject subCLBound = new BasicBSONObject();
        subCLBound.put( "LowBound", new BasicBSONObject( "a", 0 ) );
        subCLBound.put( "UpBound", new BasicBSONObject( "a", 200 ) );
        mainCL.attachCollection( csName + "." + subCLName1, subCLBound );

        int recsNum = 100;
        List< BSONObject > insertRecords = new ArrayList<>();
        for ( int i = 0; i < recsNum; i++ ) {
            BSONObject obj = new BasicBSONObject();
            obj.put( "a", i );
            obj.put( "b", i );
            insertRecords.add( obj );
        }
        mainCL.bulkInsert( insertRecords );
        mainCL.createIndex( "idx_31136", new BasicBSONObject( "b", 1 ),
                new BasicBSONObject( "Enforced", false ),
                new BasicBSONObject( "SortBufferSize", 2048 ) );

        // case1: subClName为""
        mainCL.copyIndexAsync( "", "idx_31136" );

        // case2: indexName为""
        mainCL.copyIndexAsync( csName + "." + subCLName1, "" );

        cs.dropCollection( mainClName );
    }

    @AfterClass
    public void tearDown() {
        sdb.close();
    }
}
