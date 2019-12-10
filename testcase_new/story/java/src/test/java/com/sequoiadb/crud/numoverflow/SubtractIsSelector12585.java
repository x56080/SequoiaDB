package com.sequoiadb.crud.numoverflow;

import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
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
 * FileName: SubtractIsSelector12585.java test content:Numeric value overflow
 * for single character using $subtract operation, and the $subtract is used as
 * a selector. testlink case:seqDB-12585
 * 
 * @author wuyan
 * @Date 2017.9.11
 * @version 1.00
 */

public class SubtractIsSelector12585 extends SdbTestBase {

    @DataProvider(name = "operData")
    public Object[][] generateDatas() {
        String[] expRecords1 = {
                "{'no':{'$decimal':'-9223372036854775809'},'tlong':{'$numberLong':'-9223372036854775808'},'test':0}" };
        // String []expRecords2 =
        // {"{arr:[1,0,[{'$decimal':'9223372036854775808'},1]],obj:{a:1},test:2}"};
        String[] expRecords3 = {
                "{arr:[1,0,[2147483147,1]],obj:{a:{'$decimal':'9223372036854775808'}},test:2}" };
        String[] expRecords01 = {
                "{'no':-2147483648,'tlong':{'$decimal':'-9223372036854775809'},'test':0}" };
        String[] expRecords02 = {
                "{arr:[{'$decimal':'9223372036854775808'}],obj:{a:{b:{int:2147483647,"
                        + "long:{'$numberLong':'9223372036854775800'}}}},test:1}" };
        String[] expRecords03 = {
                "{arr:[{'$numberLong':'9223372036854775807'},-2100],"
                        + "obj:{a:{b:{int:2147483647,long:{'$decimal':'9223372036854775808'}}}},test:1}" };
        String expDecimalType = "decimal";
        String expDecimalTypeToJava = "class org.bson.types.BSONDecimal";

        return new Object[][] {
                // parameters:
                // matcherValue,subValue,selectorName,expRecords,expType,isVerifyTypeToJava,
                // expTypeTojava
                // ---int32 and int64
                // no:-2147483648 $substract 9223372034707292161,the result is
                // {'$decimal':'-9223372036854775809'}
                new Object[] { 0, new Long( 9223372034707292161L ), "no",
                        expRecords1, expDecimalType, true,
                        expDecimalTypeToJava },
                // arr.$[2].$[0]:1 $substract -9223372034707292661,the result is
                // {'$decimal':'9223372036854775808'}
                // TODO:SEQUOIADBMAINSTREAM-2764
                // new Object[]{2, new Long(-9223372034707292661L),
                // "arr.$[2].$[0]",expRecords2,expDecimalType,false,null},
                // obj.a:1 $substract -9223372036854775807,the result is
                // {'$decimal':'9223372036854775808'}
                new Object[] { 2, new Long( -9223372036854775807L ), "obj.a",
                        expRecords3, expDecimalType, false, null },

                // ---int64 and int32
                // tlong:-9223372036854775808 $substract 1,the result is
                // {'$decimal':'-9223372036854775809'}
                new Object[] { 0, new Integer( 1 ), "tlong", expRecords01,
                        expDecimalType, true, expDecimalTypeToJava },
                // arr.0:9223372036854775807 $substract -1,the result is
                // {'$decimal':'9223372036854775809'}
                new Object[] { 1, new Integer( -1 ), "arr.$[0]", expRecords02,
                        expDecimalType, false, null },
                // obj.a.b.long:9223372036854775800 $substract -8,the result is
                // {'$decimal':'9223372036854775808'}
                new Object[] { 1, new Integer( -8 ), "obj.a.b.long",
                        expRecords03, expDecimalType, false, null }, };
    }

    private String clName = "subtract_selector12585";
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
                "{'no':-2147483648,'tlong':{'$numberLong':'-9223372036854775808'},'test':0}",
                "{arr:[{'$numberLong':'9223372036854775807'},-2100],obj:{a:{b:{int:2147483647,long:{'$numberLong':'9223372036854775800'}}}},test:1}",
                "{arr:[1,0,[2147483147,1]],obj:{a:1},test:2}" };

        NumOverflowUtils.insert( cl, records );
    }

    @Test(dataProvider = "operData")
    public void testSubtract( int matcherValue, Object subValue,
            String selectorName, String[] expRecords, String expTypeToSdb,
            Boolean isVerifyTypeToJava, String typeToJava ) {
        try {

            BSONObject sValue = new BasicBSONObject();
            sValue.put( "$subtract", subValue );
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
