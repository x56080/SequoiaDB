/*******************************************************************************
*@Description : query_sync testcase common functions
*@Modify list :
*              2019-5-30 wangkexin
*******************************************************************************/

/* ****************************************************
@description: insert data into collection
@parameter: 
    cl : the collection ready to insert records
    insertNum : the number of records inserted
@return: 
**************************************************** */
function insertData ( cl, insertNum )
{
    var dataArray = new Array();
    for( var i = 0; i < insertNum; i++ )
    {
        var data = { a: i };
        dataArray.push( data );
    }
    cl.insert( dataArray );
}

/* ****************************************************
@description: create a group, specify the instance id of the node
@parameter: 
    rgName : the name of dataRG
    hostName : host name of new node
@return: log paths to be backed up
**************************************************** */
function createDataGroups ( rgName, hostName, instanceidArr, logSourcePaths )
{
    var tmpArray = [];
    var dataRG = db.createRG( rgName );

    for( var i = 0; i < instanceidArr.length; i++ )
    {
        var port = parseInt( RSRVPORTBEGIN ) + ( i * 10 );
        var dataPath = RSRVNODEDIR + "data/" + port;
        var checkSucc = false;
        var times = 0;
        var maxRetryTimes = 10;
        do
        {
            try
            {
                dataRG.createNode( hostName, port, dataPath, { diaglevel: 5, instanceid: instanceidArr[i] } );
                checkSucc = true;
                var obj = new Object();
                obj.NodeName = hostName + ":" + port;
                obj.instanceid = instanceidArr[i];
                tmpArray.push( obj );
                logSourcePaths.push( hostName + ":" + CMSVCNAME + "@" + dataPath + "/diaglog/sdbdiag.log" );
            }
            catch( e )
            {
                //-145 :SDBCM_NODE_EXISTED  -290:SDB_DIR_NOT_EMPTY
                if( e == -145 || e == -290 )
                {
                    port = port + 10;
                    dataPath = RSRVNODEDIR + "data/" + port;
                }
                else
                {
                    throw "create node failed!  port = " + port + " dataPath = " + dataPath + " errorCode: " + e;
                }
                times++;
            }
        }
        while( !checkSucc && times < maxRetryTimes );
    }
    dataRG.start();
    return tmpArray;
}

/* ****************************************************
@description: remove one data group
@parameter: 
    rgName : The name of data group to be removed 
@return:
**************************************************** */
function removeDataRG ( rgName )
{
    try
    {
        db.removeRG( rgName );
    }
    catch( e )
    {
        //-154 : SDB_CLS_GRP_NOT_EXIST
        if( e !== -154 )
        {
            throw buildException( "removeDataRG()", e, "remove dataRG failed.", '-154', e );
        }
    }
}

/* ****************************************************
@description: check whether the specified node is the primary or slave node in the group
@parameter: 
    node : the specified node
    groupName : the specified group
    expMaster : if expect the node is the primary node (true/false)
@return:
**************************************************** */
function checkRole ( node, groupName, expMaster )
{
    println( "---begin to check query node[" + node + "] is master or not" );
    try
    {
        db.getRG( groupName ).getNode( node );
    }
    catch( e )
    {
        throw buildException( "checkRole()", null, "db.getRG(" + groupName + ").getNode(" + node + ")",
            "success", e );
    }

    var isMaster = new Sdb( node ).snapshot( 7 ).current().toObj().IsPrimary;
    if( isMaster !== expMaster )
    {
        throw buildException( "checkRole()", null, node + " is master node",
            expMaster, isMaster );
    }
}

/* ****************************************************
@description: check node by it instance id.
@parameter: 
    actQueryNode : the actual node
    nodeInfo : node information array  
        ex:
           [0] {"NodeName":"XXXX", "instanceid":XXXX}
           [1] {"NodeName":"XXXX", "instanceid":XXXX}
           [N] ...
    instanceid : expected instance id.
@return:
**************************************************** */
function checkNodeByInstanceId ( actQueryNode, nodeInfo, instanceid )
{
    var expNodeName = "";
    for( var i = 0; i < nodeInfo.length; i++ )
    {
        if( nodeInfo[i].instanceid == instanceid )
        {
            expNodeName = nodeInfo[i].NodeName;
            break;
        }
    }
    if( actQueryNode !== expNodeName )
    {
        throw buildException( "checkNode()", null, "check the act query node name", expNodeName, actQueryNode );
    }
}