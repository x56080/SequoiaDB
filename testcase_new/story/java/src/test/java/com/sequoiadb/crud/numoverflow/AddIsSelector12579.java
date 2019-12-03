package com.sequoiadb.crud.numoverflow;

import java.text.SimpleDateFormat;
import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDecimal;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.crud.numoverflow.NumOverflowUtils;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: AddIsSelector12579.java test content:different types of numerical
 * operations, the result of the operation is converted to a high-level type
 * testlink case:seqDB-12579
 * 
 * @author wuyan
 * @Date 2017.9.12
 * @version 1.00
 */

public class AddIsSelector12579 extends SdbTestBase {

    @DataProvider(name = "operData")
    public Object[][] generateDatas() {
        String[] expRecords1 = {
                "{'no':{'$numberLong':'-3638'},'tlong':{'$numberLong':'-6854775808'},'test':0}" };
        String[] expRecords2 = {
                "{no:-4.0,arr:[4.01],arr1:[1,'test',['0.01',{'$numberLong':'8'}]],test:1}" };
        String[] expRecords3 = {
                "{'no':-3648,'tlong':-7.15477582E9,'test':0}" };
        String[] expRecords4 = {
                "{no:{a:{c:{'$decimal':'6854778929.711'}}},obj:{a:{b:{a1:10.00,a2:3}}},test:2}" };
        String expLongType = "int64";
        String expLongTypeToJava = "class java.lang.Long";
        String[] expRecords5 = {
                "{no:-4.0,arr:[1,0,3.0],arr1:[1,'test',['0.01',9]],test:1}" };

        String expDecimalType = "decimal";
        String expDoubleType = "double";
        String expArrayType = "array";
        String expDecimalTypeToJava = "class org.bson.types.BSONDecimal";
        String expDoubleTypeToJava = "class java.lang.Double";
        return new Object[][] {
                // parameters:
                // matcherValue,subValue,selectorName,expRecords,expType,isVerifyTypeToJava,
                // expTypeTojava

                // int32 + int64
                new Object[] { 0, new Long( 10 ), "no", expRecords1,
                        expLongType, true, expLongTypeToJava },
                // int32(array) + double
                new Object[] { 1, new Double( 3.01 ), "arr.$[0]", expRecords2,
                        expDoubleType, true, expLongTypeToJava },
                // int64 + double
                new Object[] { 0, new Double( -300000012.00 ), "tlong",
                        expRecords3, expDoubleType, true, expDoubleTypeToJava },
                // int64(obj) + decimal
                new Object[] { 2, new BSONDecimal( "3121.711" ), "no.a.c",
                        expRecords4, expDecimalType, false, null },
                // int64(array) + int32 TODO:SEQUOIADBMAINSTREAM-2795
                // new Object[]{1, new Integer(1),
                // "arr1.$[2].$[1]",expRecords5,expLongType,false,null},
                // int32(obj) + double
                // new Object[]{2, new Double(0),
                // "arr1.$[2].$[1]",expRecords5,expDoubleType,false,null},

        };
    }

    private String clName = "add_selector12579";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private static DBCollection cl = null;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "connect %s failed," + coordUrl + e.getMessage() );
        }

        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = NumOverflowUtils.createCL( cs, clName );

        String[] records = {
                "{'no':-3648,'tlong':{'$numberLong':'-6854775808'},'test':0}",
                "{no:-4.0,arr:[1,0,3.0],arr1:[1,'test',['0.01',{'$numberLong':'8'}]],test:1}",
                "{no:{a:{c:{'$numberLong':'6854775808'}}},obj:{a:{b:{a1:10.00,a2:3}}},test:2}" };

        NumOverflowUtils.insert( cl, records );
    }

    @Test(dataProvider = "operData")
    public void testSubtract( int matcherValue, Object subValue,
            String selectorName, String[] expRecords, String expTypeToSdb,
            Boolean isVerifyTypeToJava, String typeToJava ) {
        try {

            BSONObject sValue = new BasicBSONObject();
            sValue.put( "$add", subValue );
            NumOverflowUtils.selectorOper( cl, matcherValue, sValue,
                    selectorName, expRecords );
            try {
                NumOverflowUtils.checkDataType( cl, sValue, matcherValue,
                        selectorName, expTypeToSdb, isVerifyTypeToJava,
                        typeToJava );
            } catch ( Exception e ) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }

        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "subtract intData is used as selector oper failed,"
                            + e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            CollectionSpace cs = sdb.getCollectionSpace( SdbTestBase.csName );
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } catch ( BaseException e ) {
            Assert.fail( "clear env failed, errMsg:" + e.getMessage() );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

}
