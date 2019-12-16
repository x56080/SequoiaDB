/************************************
*@Description: cood更新group缓存验证
*@author:      wangkexin
*@createDate:  2019.3.20
*@testlinkCase: seqDB-16647
**************************************/
import( "../clsMgr/commlib.js" );
main();
function main ()
{
    if( true == commIsStandalone( db ) )
    {
        println( "run mode is standalone" );
        return;
    }

    var clName = "cl16647";

    var groupsArray = commGetGroups( db, true, "", true, false, true );
    //获取某一数据组名称
    var originalDataRGName = groupsArray[1][0].GroupName;
    var coordNodeNum = eval( groupsArray[0].length - 1 );
    if( coordNodeNum < 2 )
    {
        println( "at least two coord nodes." );
        return;
    }

    //获得coord节点的hostname和svcname
    var hostname1 = groupsArray[0][1].HostName;
    var svcname1 = groupsArray[0][1].svcname;

    var hostname2 = groupsArray[0][2].HostName;
    var svcname2 = groupsArray[0][2].svcname;

    var db1 = null;
    var db2 = null;
    try
    {
        //coord1创建集合（新建组），coord2插入记录
        db1 = new Sdb( hostname1, svcname1 );
        var dataRGName = createDataGroups( db1, hostname1 );
        var options = { Group: dataRGName };
        var cl1 = commCreateCL( db, COMMCSNAME, clName, options, true, true );

        db2 = new Sdb( hostname2, svcname2 );
        var cl2 = db2.getCS( COMMCSNAME ).getCL( clName );
        cl2.insert( { a: 1 } );

        //coord1删除集合和组，在另一个数据组重新创建同名集合，coord2插入记录
        db1.getCS( COMMCSNAME ).dropCL( clName );
        db1.removeRG( dataRGName );
        var newOptions = { Group: originalDataRGName };
        commCreateCL( db, COMMCSNAME, clName, newOptions, true, true );

        var obj = { a: 2 };
        cl2.insert( obj );
        checkResult( cl2, obj );
    }
    finally
    {
        if( db1 != null )
        {
            db1.close();
        }
        if( db2 != null )
        {
            db2.close();
        }

        //清除环境
        commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end" );
        try
        {
            db.getRG( dataRGName );
        }
        catch( e )
        {
            if( e != -154 )
            {
                db.removeRG( dataRGName );
            }
        }
    }
}

function createDataGroups ( db, hostName )
{
    var port = parseInt( RSRVPORTBEGIN ) + 100;
    var rgName = "group16647";
    var dataRG = db.createRG( rgName );
    dataRG.createNode( hostName, port, RSRVNODEDIR + "data/" + port );
    dataRG.start();
    return rgName;
}

function checkResult ( cl, expObj )
{
    var actCount = 0;
    var cursor = cl.find();
    while( cursor.next() )
    {
        if( cursor.current().toObj()["a"] !== expObj["a"] )
        {
            throw buildException( "checkResult()", null, "query data from coord2", expObj["a"], cursor.current().toObj()["a"] );
        }
        actCount++;
    }
    if( actCount !== 1 )
        throw "number error, actCount is : " + actCount;
}