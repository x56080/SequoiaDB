package com.sequoiadb.basicoperation;

import com.sequoiadb.auth.Util;
import com.sequoiadb.base.*;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.text.ParseException;
import java.text.SimpleDateFormat;

/**
 * @description seqDB-28076 : 使用createLobID接口指定时间生成lob
 * @author Chenwenjia
 * @createDate 2022.09.29
 */
public class CreateLobId28076 extends SdbTestBase {

    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection mainCL;
    private DBCollection subCL1;
    private DBCollection subCL2;
    private SimpleDateFormat format;
    private String csName = "cs_28076";
    private String mainCLName = "cl_28076";
    private String subCLName1 = "subCL1";
    private String subCLName2 = "subCL2";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( !Util.isCluster( this.sdb ) ) {
            throw new SkipException( "Skip StandAlone" );
        }
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        cs = sdb.createCollectionSpace( csName );
        if ( cs.isCollectionExist( mainCLName ) ) {
            cs.dropCollection( mainCLName );
        }

        // main cl
        BSONObject option = new BasicBSONObject();
        option.put( "LobShardingKeyFormat", "YYYY" );
        option.put( "ShardingKey", new BasicBSONObject( "date", 1 ) );
        option.put( "ShardingType", "range" );
        option.put( "IsMainCL", true );
        mainCL = cs.createCollection( mainCLName, option );

        // sub cl
        if ( cs.isCollectionExist( subCLName1 ) ) {
            cs.dropCollection( subCLName1 );
        }
        subCL1 = cs.createCollection( subCLName1 );
        if ( cs.isCollectionExist( subCLName2 ) ) {
            cs.dropCollection( subCLName2 );
        }
        subCL2 = cs.createCollection( subCLName2 );

        // attach sub cl
        BSONObject bound1 = new BasicBSONObject();
        bound1.put( "LowBound", new BasicBSONObject( "date", "2019" ) );
        bound1.put( "UpBound", new BasicBSONObject( "date", "2020" ) );
        mainCL.attachCollection( subCL1.getFullName(), bound1 );

        BSONObject bound2 = new BasicBSONObject();
        bound2.put( "LowBound", new BasicBSONObject( "date", "2020" ) );
        bound2.put( "UpBound", new BasicBSONObject( "date", "2021" ) );
        mainCL.attachCollection( subCL2.getFullName(), bound2 );

        format = new SimpleDateFormat( "yyyy-MM-dd" );
    }

    @Test
    public void createLobIdTest() throws ParseException {
        // sub1 lob
        // make 2019 and 2020 date in the same week
        ObjectId subCL1Id = mainCL.createLobID( format.parse( "2019-12-30" ) );
        DBLob lob = mainCL.createLob( subCL1Id );
        lob.write( "sub1 test".getBytes() );
        lob.close();

        DBCursor cursor1 = subCL1.listLobs();
        DBCursor cursor2 = subCL2.listLobs();
        try {
            Assert.assertTrue( cursor1.hasNext() );
            Assert.assertFalse( cursor2.hasNext() );
        } finally {
            cursor1.close();
            cursor2.close();
        }

        // sub2 lob
        mainCL.truncate();
        ObjectId subCL2Id = mainCL.createLobID( format.parse( "2020-01-01" ) );
        lob = mainCL.createLob( subCL2Id );
        lob.write( "sub2 test".getBytes() );
        lob.close();

        cursor1 = subCL1.listLobs();
        cursor2 = subCL2.listLobs();
        try {
            Assert.assertFalse( cursor1.hasNext() );
            Assert.assertTrue( cursor2.hasNext() );
        } finally {
            cursor1.close();
            cursor2.close();
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }
}

