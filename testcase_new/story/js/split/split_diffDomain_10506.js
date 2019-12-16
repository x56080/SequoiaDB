/************************************
*@Description：切分源组和目标组不在同一domain中 
*@author：2019-3-13 wangkexin
*@testlinkCase: seqDB-10506
**************************************/
main();
function main ()
{
    var csName = "cs10506";
    var clName = "cl10506";
    var domainName = "mydomain10506";

    if( true == commIsStandalone( db ) )
    {
        println( "run mode is standalone" );
        return;
    }

    //less three groups to split
    var allGroupName = getGroupName2( db, true );
    if( 3 > allGroupName.length )
    {
        println( "--least three groups" );
        return;
    }

    var groupsInfo = getGroupName2( db, true );
    var srcGrName_a = groupsInfo[0][0];
    var tarGrName_b = groupsInfo[1][0];
    var tarGrName_c = groupsInfo[2][0];

    commDropDomain( db, domainName );
    commCreateDomain( db, domainName, [srcGrName_a, tarGrName_b], { AutoSplit: true } );
    println( "autosplit srcGrName :" + srcGrName_a + " , tarGrName : " + tarGrName_b );
    commCreateCS( db, csName, false, "", { Domain: domainName } );
    var options = { ShardingKey: { a: 1 }, ShardingType: "hash", ReplSize: 0 };
    var cl = commCreateCL( db, csName, clName, options, false );
    insertData( db, csName, clName, 100 );

    println( "--start split" );
    try
    {
        println( "srcGrName :" + srcGrName_a + " , tarGrName : " + tarGrName_c );
        cl.split( srcGrName_a, tarGrName_c, { Partition: 10 }, { Partition: 20 } )
        throw "expect fail but succeed";
    }
    catch( e )
    {
        if( e !== -216 )
        {
            throw buildException( "split()", e, "target group is not in domain", '-216', e );
        }
    }

    println( "--start check split result" );
    //查看数据切分结果，分别连接coord、源组data、目标组data查询(这里的目标组为自动切分的目标组,组b)
    var coord = new Sdb( COORDHOSTNAME, COORDSVCNAME );
    var rga_data = new Sdb( groupsInfo[0][1], groupsInfo[0][2] );
    var rgb_data = new Sdb( groupsInfo[1][1], groupsInfo[1][2] );

    try
    {
        checkCoordResult( coord, csName, clName, 100 );
        checkDataResult( rga_data, rgb_data, csName, clName, 100 );

        //再次插入数据
        insertData( db, csName, clName, 100 );
        checkCoordResult( coord, csName, clName, 200 );
        checkDataResult( rga_data, rgb_data, csName, clName, 200 );
    }
    finally
    {
        coord.close();
        rga_data.close();
        rgb_data.close();
    }

    println( "--clean cs and domain" );
    commDropCS( db, csName, true, "drop CS in the end" );
    commDropDomain( db, domainName );
}

function checkCoordResult ( db, csName, clName, expCount )
{
    var actCount = db.getCS( csName ).getCL( clName ).find().count();

    if( Number( actCount ) !== expCount )
    {
        throw buildException( "checkResult()", null, "The number of queries received by connecting coord is incorrect", expCount, actCount );
    }
}

function checkDataResult ( db1, db2, csName, clName, expCount )
{
    var actCount1 = db1.getCS( csName ).getCL( clName ).find().count();
    var actCount2 = db2.getCS( csName ).getCL( clName ).find().count();
    var actCount = actCount1 + actCount2;
    if( actCount !== expCount )
    {
        throw buildException( "checkResult()", null, "The number of queries received by connecting rga_data and rgb_data is incorrect actCount1 = " + actCount1 + " ,actCount2 = " + actCount2, expCount, actCount );
    }
}