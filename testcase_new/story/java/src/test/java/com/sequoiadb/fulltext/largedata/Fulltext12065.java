package com.sequoiadb.fulltext.largedata;

import java.util.List;

import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.testcommon.FullTestBase;

/**
 * @Description seqDB-12065: 集合空间上存在全文索引，删除集合空间
 * @author yinzhen
 * @date 2018/11/19
 */
public class Fulltext12065 extends FullTestBase {
    private String csName = "cs12065";
    private String clName = "cl12065";
    private String fullIndexName = "fullIndex12065";
    private String cappedName;
    private String esIndexName;

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );

        caseProp.setProperty( CSNAME, csName );
        caseProp.setProperty( CLNAME, clName );
    }

    // SEQUOIADBMAINSTREAM-850修改引入问题
    @Test(enabled = false)
    public void test() throws Exception {
        // 在集合上创建1个全文索引，并插入包含索引字段的数据
        cl.createIndex( fullIndexName,
                "{\"a\":\"text\",\"b\":\"text\",\"c\":\"text\",\"d\":\"text\",\"e\":\"text\",\"f\":\"text\"}",
                false, false );
        FullTextDBUtils.insertData( cl, FullTextUtils.INSERT_NUMS );
        Assert.assertTrue( FullTextUtils.isIndexCreated( cl, fullIndexName,
                FullTextUtils.INSERT_NUMS ) );

        // 直连集合所在的数据节点主节点，使用游标的方式获取对应的固定集合中的一条记录
        List< DBCollection > cappedCLs = FullTextDBUtils.getCappedCLs( cl,
                fullIndexName );
        DBCollection cappedCL = cappedCLs.get( 0 );
        DBCursor cursor = cappedCL.query();
        cursor.getNext();

        // 多次执行删除集合空间的操作
        if ( cappedCL.getCount() > 2 ) {
            for ( int i = 0; i < 3; i++ ) {
                try {
                    sdb.dropCollectionSpace( csName );
                    Assert.fail( "drop cs need to return -147!" );
                } catch ( BaseException e ) {
                    Assert.assertEquals( e.getErrorCode(), -147,
                            e.getMessage() );
                }
            }
        }

        // 关闭打开的游标
        if ( cursor != null ) {
            cursor.close();
        }

        // 关闭步骤2中打开的游标后，再次删除集合空间
        cappedName = FullTextDBUtils.getCappedName( cl, fullIndexName );
        esIndexName = FullTextDBUtils.getESIndexName( cl, fullIndexName );
        FullTextDBUtils.dropCollectionSpace( sdb, csName );
        Assert.assertTrue(
                FullTextUtils.isIndexDeleted( sdb, esIndexName, cappedName ) );
    }

    @Override
    protected void caseFini() throws Exception {
        Assert.assertTrue(
                FullTextUtils.isIndexDeleted( sdb, esIndexName, cappedName ) );
    }
}
