/************************************
*@Description: 集合中存在全文索引，修改普通集合的副本数
*@author:      liuxiaoxuan
*@createdate:  2019.08.21
*@testlinkCase: seqDB-12079
**************************************/

function main ()
{
    if( commIsStandalone( db ) ) { return; }

    var clName = COMMCLNAME + "_ES_12079";
    commDropCL( db, COMMCSNAME, clName, true, true );

    var dbcl = commCreateCL( db, COMMCSNAME, clName );

    // 创建全文索引，插入数据
    var textIndexName = "textIndex_12079";
    dbcl.createIndex( textIndexName, { "a": "text" } );
    var objs = new Array();
    for( var i = 0; i < 10000; i++ )
    {
        objs.push( { a: "test_12079 " + i, b: i } );
    }
    dbcl.insert( objs );

    // 修改普通集合的副本数为3
    db.getCS( COMMCSNAME ).getCL( clName ).alter( { ReplSize: 3 } );

    // 停止其中一个节点，插入记录
    var groups = commGetCLGroups( db, COMMCSNAME + "." + clName );
    var preSlave = db.getRG( groups[0] ).getSlave();
    try
    {
        preSlave.stop();
        dbcl.insert( { a: 'aaaaaaaaaaaaaaaa' } );
        throw "should insert fail";
    }
    catch( e )
    {
        if( e != -105 && e != -252 )
        {
            throw buildException( "insert()", e, "insert", "-105 or -252", e );
        }
    }
    finally
    {
        preSlave.start();
        // 节点起来后，检查数据组所有节点LSN是否一致
        checkGroupBusiness( 120, COMMCSNAME, clName );
    }

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