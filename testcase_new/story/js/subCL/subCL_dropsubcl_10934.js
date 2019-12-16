/************************************
*@Description:  不同coord节点删除同一张子表
*@author:      wangkexin
*@createDate:  2019.3.12
*@testlinkCase: seqDB-10934
**************************************/
main();
function main ()
{
    var mainCL_Name = "maincl10934";
    var subCL_Name = "subcl10934";
    var clNum = 10;

    if( true == commIsStandalone( db ) )
    {
        println( "run mode is standalone" );
        return;
    }

    var db1 = null;
    var db2 = null;
    try
    {
        //连接1挂载子表后删除子表
        db1 = new Sdb( COORDHOSTNAME, COORDSVCNAME );

        var mainCLOption = { ShardingKey: { "a": 1 }, ShardingType: "range", IsMainCL: true };
        var maincl = commCreateCL( db1, COMMCSNAME, mainCL_Name, mainCLOption, true, true );

        var subClOption = { ShardingKey: { "b": 1 }, ShardingType: "hash", AutoSplit: true, ReplSize: 0 };
        commCreateCL( db1, COMMCSNAME, subCL_Name, subClOption, true, true );

        var options = { LowBound: { a: 1 }, UpBound: { a: 100 } };
        maincl.attachCL( COMMCSNAME + "." + subCL_Name, options );

        maincl.insert( { a: 10 } );

        db1.getCS( COMMCSNAME ).dropCL( subCL_Name );

        //连接2再次删除子表，并通过主表查询
        db2 = new Sdb( COORDHOSTNAME, COORDSVCNAME );
        try
        {
            db2.getCS( COMMCSNAME ).dropCL( subCL_Name );
            throw "expect fail but success!";
        }
        catch( e )
        {
            if( e != -23 )
            {
                throw buildException( "dropCL()", e, "drop collection should fail", '-23', e );
            }
        }

        var cursor = db2.getCS( COMMCSNAME ).getCL( mainCL_Name ).find();
        if( cursor.next() != null )
        {
            throw buildException( "find", null, "check record", "no data", "have data" );
        }
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
    }

    //清除环境
    commDropCL( db, COMMCSNAME, mainCL_Name, true, true, "drop CL in the end" );
}