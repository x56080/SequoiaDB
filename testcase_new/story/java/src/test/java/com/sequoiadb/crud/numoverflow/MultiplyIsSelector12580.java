package com.sequoiadb.crud.numoverflow;

import java.text.SimpleDateFormat;
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
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: MultiplyIsSelector12580.java test content:Numeric value overflow
 * for single character using $multiply operation, and the $multiply is used as
 * a selector. testlink case:seqDB-12580
 * 
 * @author luweikang
 * @Date 2017.9.11
 * @version 1.00
 */

public class MultiplyIsSelector12580 extends SdbTestBase {

    private String clName = "multiply_selector12580";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private static DBCollection cl = null;

    @DataProvider(name = "operData")
    public Object[][] generateIntDatas() {
        String[] expRecords1 = {
                "{'no':2147483648,'tlong':{'$numberLong':'-9223372036854775808'},'test':0}" };
        String[] expRecords2 = {
                "{'no':-2147483648,'tlong':{'$decimal':'9223372036854775808'},'test':0}" };
        String[] expRecords3 = {
                "{'no':2147483647,'long':{'$decimal':'-18446744073709551614'},test:1}" };
        String[] expRecords4 = { "{'no':246.0,'double':123.5,'test':2}" };
        String[] expRecords5 = { "{'no':123.0,'double':247.0,'test':2}" };
        String[] expRecords6 = {
                "{'no':[2147483147,{'$numberLong':'8223372036854775807'}],'arr':[2147483648],'obj':{a:'123'},'test':3}" };
        String[] expRecords7 = {
                "{'no':[2147483147,{'$numberLong':'8223372036854775807'}],'arr':[1000000000,-2147483648],'obj':null,'test':3}" };
        String expJavaLong = "class java.lang.Long";
        String expJavaDouble = "class java.lang.Double";
        String expJavaDecimal = "class org.bson.types.BSONDecimal";
        String expLongType = "int64";
        String expDoubleType = "double";
        String expDecimalType = "decimal";

        return new Object[][] {
                // the parameters:
                // matcherValue,subValue,selectorName,expRecords,expType,isVerifyDataType,expTypeToJava
                // -2147483648 $multiply -1 the result is 2147483648(int64)
                new Object[] { 0, new Integer( -1 ), "no", expRecords1,
                        expLongType, true, expJavaLong },
                // -9223372036854775808 $multiply -1 the result is
                // 9223372036854775808(decimal)
                new Object[] { 0, new Integer( -1 ), "tlong", expRecords2,
                        expDecimalType, true, expJavaDecimal },
                // 9223372036854775807 $multiply -2 the result is
                // {'$decimal':'-18446744073709551614'}
                new Object[] { 1, new Long( -2 ), "long", expRecords3,
                        expDecimalType, true, expJavaDecimal },
                // 123.0 $multiply 2 the result is 246.0
                new Object[] { 2, new Integer( 2 ), "no", expRecords4,
                        expDoubleType, true, expJavaDouble },
                // 123.5 $multiply 2(int64) the result is 247.0
                new Object[] { 2, new Integer( 2 ), "double", expRecords5,
                        expDoubleType, true, expJavaDouble },
                // arr $multiply -1 the result is [2147483648]
                new Object[] { 3, new Integer( -1 ), "arr.$[1]", expRecords6,
                        expLongType, false, expJavaLong },
                // obj $multiply 1 the result is null
                new Object[] { 3, new Integer( -1 ), "obj", expRecords7, "null",
                        false, null }, };
    }

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
                "{'no':2147483647,'long':{'$numberLong':'9223372036854775807'},'test':1}",
                "{'no':123.0,'double':123.5,'test':2}",
                "{'no':[2147483147,{'$numberLong':'8223372036854775807'}],'arr':[1000000000,-2147483648],'obj':{a:'123'},'test':3}" };

        NumOverflowUtils.insert( cl, records );
    }

    @Test(dataProvider = "operData")
    public void testMultiply( int matcherValue, Object mulValue,
            String selectorName, String[] expRecords, String expType,
            Boolean isVerifyTypeToJava, String expTypeToJava ) {
        try {
            BSONObject mValue = new BasicBSONObject();
            mValue.put( "$multiply", mulValue );
            NumOverflowUtils.selectorOper( cl, matcherValue, mValue,
                    selectorName, expRecords );
            try {
                NumOverflowUtils.checkDataType( cl, mValue, matcherValue,
                        selectorName, expType, isVerifyTypeToJava,
                        expTypeToJava );
            } catch ( Exception e ) {
                e.printStackTrace();
            }

        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "multiply data is used as selector oper failed,"
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
