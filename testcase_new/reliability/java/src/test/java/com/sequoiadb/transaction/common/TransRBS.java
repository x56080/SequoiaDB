/**
 * 
 */
/**
 * Copyright (c) 2020, SequoiaDB Ltd.
 * File Name:TransRBS.java
 * 类的详细描述
 *
 *  @author 类创建者姓名
 * Date:2020年1月31日下午3:07:39
 *  @version 1.00
 */
package com.sequoiadb.transaction.common;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;

/**
 * @author zhaoyu
 *
 */

public class TransRBS {
    public static List< BSONObject > insertDatas( DBCollection cl ) {
        List< BSONObject > records = new ArrayList<>();
        StringBuilder sb = new StringBuilder();
        for ( int i = 0; i < 100; i++ ) {
            sb.append( "a" );
        }

        for ( int i = 0; i < 10000; i++ ) {
            records.add( ( BSONObject ) JSON.parse( "{_id:" + i + ",a:" + i
                    + ",b:" + i + ",c:'" + sb + "'}" ) );
        }
        List< BSONObject > expList = new ArrayList<>( records );
        cl.insert( records );
        return expList;
    }

    public static void genMultiRBSCL( DBCollection cl ) {
        Sequoiadb db = cl.getSequoiadb();
        for ( int i = 0; i < 2; i++ ) {
            db.beginTransaction();
            cl.update( null, "{$inc:{a:1}}", "{'':'a'}" );
            db.commit();
            cl.update( null, "{$inc:{a:1}}", "{'':'a'}" );
            db.beginTransaction();
            cl.update( null, "{$inc:{a:1}}", "{'':'a'}" );
            db.commit();
            System.out.println( "i:" + i );
        }
    }

}
