/************************************
*@Description: replSize设置不为1时插入/更新/删除记录
*@author:      liuxiaoxuan
*@createdate:  2019.08.21
*@testlinkCase: seqDB-12010
**************************************/

function main ()
{
    if( commIsStandalone( db ) ) { return; }

    var clName = COMMCLNAME + "_ES_12010";
    commDropCL( db, COMMCSNAME, clName, true, true );

    var dbcl = commCreateCLByOption( db, COMMCSNAME, clName, { ReplSize: 7 } );

    // 创建全文索引，插入数据
    var textIndexName = "textIndex_12010";
    dbcl.createIndex( textIndexName, { "a": "text" } );
    dbcl.insert( { a: 'insertBeforeNodeStop' } );

    // 检查数据同步
    checkFullSyncToES( COMMCSNAME, clName, textIndexName, 1 );

    // 停止其中一个节点，插入记录
    var groups = commGetCLGroups( db, COMMCSNAME + "." + clName );
    var preSlave = db.getRG( groups[0] ).getSlave();
    try
    {
        preSlave.stop();

        try
        {
            dbcl.insert( { a: 'insertAfterNodeStop' } );
            throw "should insert fail";
        }
        catch( e )
        {
            if( e != -105 && e != -252 )
            {
                throw buildException( "insert()", e, "insert", "-105 or -252", e );
            }
        }

        try
        {
            dbcl.update( { $set: { a: 'updateAfterNodeStop' } } );
            throw "should udpate fail";
        }
        catch( e )
        {
            if( e != -105 && e != -252 )
            {
                throw buildException( "udpate()", e, "udpate", "-105 or -252", e );
            }
        }

        try
        {
            dbcl.remove();
            throw "should remove fail";
        }
        catch( e )
        {
            if( e != -105 && e != -252 )
            {
                throw buildException( "remove()", e, "remove", "-105 or -252", e );
            }
        }
    }
    finally
    {
        preSlave.start();
        // 节点起来后，检查数据组所有节点LSN是否一致
        checkGroupBusiness( 120, COMMCSNAME, clName );
    }

    // 节点起来后，再次插入一条记录
    dbcl.insert( { a: 'insertAfterNodeStart' } );

    // 检查数据同步
    checkFullSyncToES( COMMCSNAME, clName, textIndexName, dbcl.count() );
    checkConsistency( COMMCSNAME, clName );

    var actCount = dbcl.find( { "": { "$Text": { "query": { "match_all": {} } } } } ).count();
    var expectCount = dbcl.count();
    if( parseInt( actCount ) != parseInt( expectCount ) )
    {
        println( "expect count: " + parseInt( expectCount ) + ", actual count: " + parseInt( actCount ) );
        throw "check count fail";
    }

    var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, textIndexName );
    commDropCL( db, COMMCSNAME, clName, true, true );
    //SEQUOIADBMAINSTREAM-3983
    checkIndexNotExistInES( esIndexNames );
}
try
{
    main();
}
catch( e )
{
    if( e.constructor === Error )
    {
        println( e.stack );
    }
    throw e;
}
;