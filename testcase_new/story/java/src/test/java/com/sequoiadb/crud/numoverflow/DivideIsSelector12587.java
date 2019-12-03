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
 * FileName: DivideIsSelector12587.java test content:Numeric value overflow for
 * single character using $divide operation, and the $divide is used as a
 * selector. testlink case:seqDB-12587
 * 
 * @author luweikang
 * @Date 2017.9.12
 * @version 1.00
 */

public class DivideIsSelector12587 extends SdbTestBase {

    private String clName = "divide_selector12587";
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
                "{'no':{'$numberLong':'-214748364'},'long':{'$numberLong':'9223372036854775807'},test:1}" };
        String[] expRecords4 = {
                "{'no':2147483647,'long':{'$numberLong':'-922337203685477580'},test:1}" };
        String[] expRecords5 = { "{'no':[82233720368547758070.0],'test':2}" };
        String[] expRecords6 = {
                "{'no':[[1024]],obj:{a:{b:{'$numberLong':'1073741573'}}},'test':3}" };
        String expJavaLong = "class java.lang.Long";
        String expJavaDouble = "class java.lang.Double";
        String expJavaDecimal = "class org.bson.types.BSONDecimal";
        String expLongType = "int64";
        String expDoubleType = "double";
        String expDecimalType = "decimal";

        return new Object[][] {
                // the parameters:
                // matcherValue,subValue,selectorName,expRecords,expType,isVerifyDataType,expTypeToJava
                new Object[] { 0, new Integer( -1 ), "no", expRecords1,
                        expLongType, true, expJavaLong },
                new Object[] { 0, new Integer( -1 ), "tlong", expRecords2,
                        expDecimalType, true, expJavaDecimal },
                new Object[] { 1, new Long( -10 ), "no", expRecords3,
                        expLongType, true, expJavaLong },
                new Object[] { 1, new Long( -10 ), "long", expRecords4,
                        expLongType, true, expJavaLong },
                new Object[] { 2, new Double( 0.1 ), "no.$[1]", expRecords5,
                        expDoubleType, true, expJavaDouble },
                new Object[] { 3, new Long( 2 ), "obj.a.b", expRecords6,
                        expLongType, false, null }, };
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
                "{'no':[2147483147,{'$numberLong':'8223372036854775807'}],'test':2}",
                "{'no':[[1024]],obj:{a:{b:2147483147}},'test':3}" };

        NumOverflowUtils.insert( cl, records );
    }

    @Test(dataProvider = "operData")
    public void testDivide( int matcherValue, Object mulValue,
            String selectorName, String[] expRecords, String expType,
            Boolean isVerifyTypeToJava, String expTypeToJava ) {
        try {
            BSONObject mValue = new BasicBSONObject();
            mValue.put( "$divide", mulValue );
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
                    "divide data is used as selector oper failed,"
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
