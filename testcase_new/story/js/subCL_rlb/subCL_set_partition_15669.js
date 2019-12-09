/************************************
*@Description: 指定domain创建子表，设置分区数小于domain所包含组个数 
*@author:      wangkexin
*@createDate:  2019.3.11
*@testlinkCase: seqDB-15669
**************************************/
main();
function main ()
{
    var csName = "cs15669";
    var mainCL_Name = "maincl15669";
    var subCL_Name = "subcl15669";
    var domainName = "domain15669";

    if( true == commIsStandalone( db ) )
    {
        println( "run mode is standalone." );
        return;
    }

    var groupsArray = commGetGroups( db, false, "", true, true, true );
    var dataRGNum = groupsArray.length;
    if( dataRGNum < 3 )
    {
        println( "at least three data groups." );
        return;
    }

    var hostNames = [];
    for( var i = 1; i < groupsArray[0].length; i++ )
    {
        hostNames.push( groupsArray[0][i].HostName );
    }

    //新创建6个组
    var newDataRGNames = createDataGroups( db, hostNames, 6 );

    try
    {
        //将新建的6个组和环境中原有的3个（或3个以上）的组名一起存放在totalDataRGNames中
        var totalDataRGArray = commGetGroups( db, false, "", true, true, true );
        var totalDataRGNames = [];
        for( var i = 0; i < totalDataRGArray.length; i++ )
        {
            totalDataRGNames.push( totalDataRGArray[i][0].GroupName );
        }

        commDropDomain( db, domainName );
        commCreateDomain( db, domainName, totalDataRGNames, { AutoSplit: true } );
        commCreateCS( db, csName, true, "", { Domain: domainName } );

        println( "start to create main cl." );
        var mainCLOption = { ShardingKey: { "a": 1 }, ShardingType: "range", IsMainCL: true };
        var dbcl = commCreateCLByOption( db, csName, mainCL_Name, mainCLOption, true, true );

        println( "start to create sub cl." );
        var subClOption = { ShardingKey: { "b": 1 }, ShardingType: "hash", AutoSplit: true, Partition: 8, ReplSize: 0 };
        try
        {
            db.getCS( csName ).createCL( subCL_Name, subClOption );
            throw "expect fail but success.";
        } catch( e )
        {
            if( e !== -6 )
            {
                throw buildException( "main()", e, "create subCL", '-6', e );
            }
        }
    }
    finally
    {
        //清除环境
        commDropCS( db, csName, true, "drop CS in the end" );
        removeDataRG( db, newDataRGNames );
        commDropDomain( db, domainName );
    }
}

function createDataGroups ( db, hostNames, groupNum )
{
    var dataGroupNames = [];
    var rgName = "";
    try
    {
        var hostNameIndex = 0;
        for( var i = 0; i < groupNum; i++ )
        {
            var port = parseInt( RSRVPORTBEGIN ) + ( i * 10 );
            rgName = "group15669_" + i;
            var dataRG = db.createRG( rgName );
            dataGroupNames.push( rgName );
            if( hostNameIndex == hostNames.length )
            {
                hostNameIndex = 0;
            }
            dataRG.createNode( hostNames[hostNameIndex], port, RSRVNODEDIR + "data/" + port );
            dataRG.start();
            hostNameIndex++;
        }
        return dataGroupNames;
    }
    catch( e )
    {
        removeDataRG( db, dataGroupNames );
        throw "create data RG " + rgName + " failed , errorCode = " + e;
    }
}

function removeDataRG ( db, dataRGNames )
{
    for( var i = 0; i < dataRGNames.length; i++ )
    {
        db.removeRG( dataRGNames[i] );
    }
}