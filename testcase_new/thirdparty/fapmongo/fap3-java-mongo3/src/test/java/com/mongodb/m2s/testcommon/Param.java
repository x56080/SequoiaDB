package com.mongodb.m2s.testcommon;

import java.util.ArrayList;

import com.alibaba.fastjson.JSONArray;
import com.alibaba.fastjson.JSONObject;
import org.testng.Assert;

/**
 * @Descreption
 * @Author
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/21
 * @UpdateRemark
 * @Version
 */
public class Param {
    private String name;
    private int count;
    private ArrayList< Param > subParam;

    public Param() {
        this.name = "";
        this.count = 0;
        this.subParam = new ArrayList< Param >();
    }

    public Param( String name, int count ) {
        this.name = name;
        this.count = count;
        this.subParam = new ArrayList< Param >();
    }

    public Param( String name, int count, ArrayList< Param > subParam ) {
        this.name = name;
        this.count = count;
        this.subParam = subParam;
    }

    public String getName() {
        return name;
    }

    public void setName( String name ) {
        this.name = name;
    }

    public int getCount() {
        return count;
    }

    public void setCount( int count ) {
        this.count = count;
    }

    public ArrayList< Param > getSubParam() {
        return subParam;
    }

    public void setSubParam( ArrayList< Param > subParam ) {
        this.subParam = subParam;
    }

    public void addSubParam( Param par ) {
        this.subParam.add( par );
    }

    public static void checkRecordParams( JSONObject record, String cmdName,
            int count, Param messageParam, Param dbCmdParam, Param opParam ) {
        // verify cmdName and count
        Assert.assertEquals( record.get( "databaseCmd" ), cmdName,
                cmdName + " param name is not equal" );
        Assert.assertEquals( record.get( "count" ), count,
                cmdName + " param count is not equal" );

        // TODO OP_CODE verify

        // verify message
        if ( messageParam.getName().equals( "messageParameters" ) ) {
            JSONArray msgParam = record.getJSONArray( "messageParameters" );
            Assert.assertTrue(
                    checkParamArrays( msgParam, messageParam.getSubParam() ),
                    cmdName + " message param is not equal" );
        }

        // verify dbCmd
        if ( dbCmdParam.getName().equals( "databaseCmdParameters" ) ) {
            JSONArray dbCmd = record.getJSONArray( "databaseCmdParameters" );
            Assert.assertTrue(
                    checkParamArrays( dbCmd, dbCmdParam.getSubParam() ),
                    cmdName + " dbCmd param is not equal" );
        }

        // verify operation
        if ( opParam.getName().equals( "operators" ) ) {
            JSONArray op = record.getJSONArray( "operators" );
            Assert.assertTrue( checkParamArrays( op, opParam.getSubParam() ),
                    cmdName + " operation param is not equal" );
        }
    }

    public static Boolean checkParamArrays( JSONArray arr,
            ArrayList< Param > sub ) {
        if ( arr.size() == 0 ) {
            return false;
        }
        for ( int i = 0; i < sub.size(); ++i ) {
            Boolean flag = false;
            Param subPar = sub.get( i );
            for ( int j = 0; j < arr.size(); ++j ) {
                JSONObject obj = arr.getJSONObject( j );
                if ( obj.get( "param" ).equals( subPar.getName() ) ) {
                    // if had subParams, there is no count info
                    if ( subPar.getSubParam().size() != 0 ) {
                        JSONArray subArr = obj.getJSONArray( "operators" );
                        flag = checkOpSubParamArrays( subArr,
                                subPar.getSubParam() );
                    } else if ( obj.get( "count" )
                            .equals( subPar.getCount() ) ) {
                        flag = true;
                    }
                    break;
                }
            }
            if ( !flag ) {
                return false;
            }
        }
        return true;
    }

    public static Boolean checkOpSubParamArrays( JSONArray arr,
            ArrayList< Param > sub ) {
        if ( arr.size() == 0 ) {
            return false;
        }
        for ( int i = 0; i < sub.size(); ++i ) {
            Boolean flag = false;
            Param subPar = sub.get( i );
            for ( int j = 0; j < arr.size(); ++j ) {
                JSONObject obj = arr.getJSONObject( j );
                if ( obj.get( "operator" ).equals( subPar.getName() ) ) {
                    // if had subParams, there is no count info
                    if ( subPar.getSubParam().size() != 0 ) {
                        JSONArray subArr = obj.getJSONArray( "subOperators" );
                        if ( !checkOpSubParamArrays( subArr,
                                subPar.getSubParam() ) ) {
                            return false;
                        }
                    }
                    if ( !obj.get( "count" ).equals( subPar.getCount() ) ) {
                        return false;
                    }
                    break;
                }
            }
        }
        return true;
    }

}
