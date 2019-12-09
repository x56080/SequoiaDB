/*************************************************************
* @Description: reloadConf then create catalog node 
*               check if conf file will double config or not
* testcase for Jira Questionaire: SEQUOIADBMAINSTREAM-2371
*               seqDB-11722:reloadConf后新建编目节点
* @Author:      Liangxw Init
*************************************************************/

// get all groups include SYSCoord and SYSCatalogGroup, 
// return arr [ "group1", "group2", ... ]
function getAllGroups ()
{
    var groups = new Array();
    var cursor = db.listReplicaGroups();
    var tmpInfo;
    while( tmpInfo = cursor.next() )
    {
        var groupname = tmpInfo.toObj().GroupName;
        groups.push( groupname );
    }
    return groups;
}

// get nodes in group
// return arr [ "sdbserver1:20100", ... ]
function getGroupNodes ( groupname )
{
    var arr = new Array();
    var tmpObj = db.getRG( groupname ).getDetail().next().toObj();
    var tmpGroupArray = tmpObj["Group"];
    for( var j = 0; j < tmpGroupArray.length; ++j )
    {
        var tmpNodeObj = tmpGroupArray[j];
        var nodename = tmpNodeObj["HostName"];

        for( var k = 0; k < tmpNodeObj.Service.length; ++k )
        {
            var tmpSvcObj = tmpNodeObj.Service[k];
            if( tmpSvcObj["Type"] == 0 )
            {
                nodename = nodename + ":" + tmpSvcObj["Name"];
                arr.push( nodename );
                break;
            }
        }
    }

    return arr;
}

// set node config with key value
// it will change node conf file
function setNodeConfig ( node, key, value )
{
    var hostname = node.split( ":" )[0];
    var svcname = node.split( ":" )[1];
    var oma = new Oma( hostname, CMSVCNAME );
    var configs = oma.getNodeConfigs( svcname ).toObj();
    configs[key] = value;
    oma.setNodeConfigs( svcname, configs );
    // check 
    var result = oma.getNodeConfigs( svcname ).toObj()[key];
    println( "node: " + node + " " + key + " = " + result );
    oma.close();
}

// check node conf file have two same configs or not 
function checkNodeConfFile ( node )
{
    var hostname = node.split( ":" )[0];
    var svcname = node.split( ":" )[1];
    var remote = new Remote( hostname, CMSVCNAME );
    var cmd = remote.getCmd();
    var sdbDir = toolGetSequoiadbDir( hostname, CMSVCNAME );
    var confFile = sdbDir[0] + "/conf/local/" + svcname + "/sdb.conf";

    var configArr = cmd.run( "cat " + confFile ).split( "\n" );
    if( checkArrDupElement( configArr ) )
    {
        throw buildException( "checkNodeConfFile", 0,
            "check node: " + node + " conf file: " + confFile,
            "no duplicate conf", "have duplicate conf" );
    }
    remote.close();
}

// check array have two same element or not
// return true if have, return false if not have
function checkArrDupElement ( arr )
{
    arr.sort();
    for( var i = 0; i < arr.length - 1; i++ )
    {
        if( arr[i] == arr[i + 1] )
        {
            println( "duplicate element: " + arr[i] );
            return true;
        }
    }
    return false;
}

function testNodeConfig ( key, value ) 
{
    // get a data group
    var groups = getAllGroups();
    var group;
    for( var i = 0; i < groups.length; i++ )
    {
        if( groups[i] == "SYSCoord" || groups[i] == "SYSCatalogGroup" )
            continue;
        group = groups[i];
        break;
    }
    var rg = db.getRG( group );
    println( "success to get data group: " + group );

    // create node in group and start
    var nodes = getGroupNodes( group );
    var hostname = nodes[0].split( ":" )[0];
    var svcname = toolGetIdleSvcName( hostname, CMSVCNAME );
    var node = hostname + ":" + svcname;
    var dbpath = RSRVNODEDIR + "data/" + svcname;
    var srcLogPath = "";
    try
    {
        rg.createNode( hostname, svcname, dbpath, { diaglevel: 5 } );
        println( "success to create node " + node );
        srcLogPath = hostname + ":" + CMSVCNAME + "@" + dbpath + "/diaglog/sdbdiag.log";
        rg.start();
        println( "success to start group: " + group );

        // set node config and reload conf 
        setNodeConfig( node, key, value );
        println( "success to set node config: " + key + "=" + value );
        db.reloadConf();
        println( "success to reload conf" );
    }
    catch( e )
    {
        println( "catch e : " + e );
        //将新建data节点日志备份到/tmp/ci/rsrvnodelog目录下
        var backupDir = "/tmp/ci/rsrvnodelog/11722";
        File.mkdir( backupDir );
        File.scp( srcLogPath, backupDir + "/sdbdiag.log" );
        throw e;
    }
    finally
    {
        // remove node
        rg.removeNode( hostname, svcname );
        println( "success to remove node " + node );
    }

    var srcLogCoordPath = "";
    try
    {
        // create a cata node and start and remove
        var cataRg = db.getRG( "SYSCatalogGroup" );
        dbpath = RSRVNODEDIR + "catalog/" + svcname;
        cataRg.createNode( hostname, svcname, dbpath );
        srcLogCoordPath = hostname + ":" + CMSVCNAME + "@" + dbpath + "/diaglog/sdbdiag.log";
        println( "success to create cata node " + node );
        cataRg.start();
        println( "success to start catalog group" );
    }
    catch( e )
    {
        println( "catch e : " + e );
        //将新建coord节点日志备份到/tmp/ci/rsrvnodelog目录下
        var backupDir = "/tmp/ci/rsrvnodelog/11722_coord";
        File.mkdir( backupDir );
        File.scp( srcLogCoordPath, backupDir + "/sdbdiag.log" );
        throw e;
    }
    finally
    {
        cataRg.removeNode( hostname, svcname );
        println( "success to remove cata node " + node );
    }

    // check all nodes conf file
    var groups = getAllGroups();
    for( i = 0; i < groups.length; i++ )
    {
        nodes = getGroupNodes( groups[i] );
        for( var j = 0; j < nodes.length; j++ )
        {
            checkNodeConfFile( nodes[j] );
        }
    }
}


function main ()
{
    if( commIsStandalone( db ) )
    {
        println( "Run mode is standalone" );
        return;
    }
    testNodeConfig( "weight", 20 );
    // testNodeConfig( "dbpath", "123456789" ) ;
}

main();