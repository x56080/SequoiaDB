package com.sequoiadb.transaction.serial;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description: seqDB-19197:无事务操作时，清空会话缓存中的事务配置
 * @Author wangkexin
 * @Date 2019.09.04
 */

public class TestSessionAttr19197 extends SdbTestBase {
    private BSONObject defaultValue;

    @BeforeClass
    public void setup() {
        defaultValue = new BasicBSONObject();
        defaultValue.put( "PreferedInstance", "M" );
        defaultValue.put( "PreferedInstanceMode", "random" );
        defaultValue.put( "PreferedStrict", false );
        defaultValue.put( "PreferedPeriod", 60 );
        defaultValue.put( "Timeout", -1 );
        defaultValue.put( "TransIsolation", 0 );
        defaultValue.put( "TransTimeout", 60 );
        defaultValue.put( "TransUseRBS", true );
        defaultValue.put( "TransLockWait", false );
        defaultValue.put( "TransAutoCommit", false );
        defaultValue.put( "TransAutoRollback", true );
        defaultValue.put( "TransRCCount", true );
        defaultValue.put( "Source", "" );
    }

    @Test
    public void test19196() {
        Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        try {
            // 获取会话默认事务属性
            checkSessionAttr( db, defaultValue, null );

            // 设置会话事务隔离级别为RS
            BSONObject config = new BasicBSONObject();
            config.put( "TransIsolation", 2 );
            db.setSessionAttr( config );

            BasicBSONObject tmpConf = new BasicBSONObject(
                    ( BasicBSONObject ) defaultValue );
            tmpConf.put( "TransIsolation", 2 );
            checkSessionAttr( db, tmpConf, null );

            // 设置会话事务隔离级别为RU，超时时间为120秒
            BSONObject updateConfig = new BasicBSONObject();
            updateConfig.put( "transisolation", 0 );
            updateConfig.put( "transactiontimeout", 120 );
            db.updateConfig( updateConfig );

            // 清空缓存
            db.setSessionAttr( new BasicBSONObject() );

            // 获取该会话上的事务的缓存
            tmpConf.put( "TransTimeout", 120 );
            checkSessionAttr( db, tmpConf, null );

            // 获取该会话上的事务属性
            checkSessionAttr( db, tmpConf, false );
        } finally {
            BSONObject updateConfig = new BasicBSONObject();
            updateConfig.put( "transisolation", 0 );
            updateConfig.put( "transactiontimeout", 60 );
            db.updateConfig( updateConfig );
            db.close();
        }
    }

    @AfterClass
    public void teardown() {
    }

    private void checkSessionAttr( Sequoiadb db, BSONObject expSessionAttr,
            Boolean useCache ) {
        BSONObject sessionAttr;
        if ( useCache != null ) {
            sessionAttr = db.getSessionAttr( useCache );
        } else {
            sessionAttr = db.getSessionAttr();
        }
        Assert.assertEquals( sessionAttr.toString(),
                expSessionAttr.toString() );
    }
}
