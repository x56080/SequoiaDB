/**************************************
 * @Description: 测试用例 seqDB-10939 :: 版本: 1 :: 指定options参数为GroupID/GroupName
 * @author: ouyangzhongnan
 * @Date: 2017-01-05
 * @RunDemo:
 * sdb -f "func.js,commlib.js,forceSession_10939.js" -e "var CHANGEDPREFIX='PREFIX';var COORDHOSTNAME='sdbserver01';var COORDSVCNAME='11810'"
 **************************************/

/*
 步骤：
 1、连接coord节点
 2、通过list()获取当前集群session（如db.list(3) )）
 3、执行db.forceSession(sessionID,options)终止会话，options指定组信息，分别验证如下情况：
 a、options指定GroupID、NodeID、 HostName
 b、options指定GroupName、svcname、 HostName
 4、查看操作结果
 结果：
 指定sessionID被终止查看对应的sessionID已经不存在[备注，指定sessionID依然存在2017-01-18改的代码]
 */

main();
function main ()
{

    if( commIsStandalone( db ) ) return;

    // 需要先list(2)一下， 然后list(3)采用连接
    var temp = db.list( 2 );

    /**3.a*/
    var sessionList = db.list( 3 ).toArray();
    var forceSession = JSON.parse( sessionList[0] );
    var NodeName = forceSession.NodeName;
    var SessionId = forceSession.SessionID;
    var HostName = NodeName.split( ":" )[0];
    var infoByNodeName = new InfoByNodeName( NodeName );
    var GroupId = infoByNodeName.getGroupIDByNodeName();
    var NodeId = infoByNodeName.getNodeIdByNodeName();
    try
    {
        db.forceSession( SessionId, { GroupId: GroupId, NodeId: NodeId, HostName: HostName } );
    } catch( e )
    {
        throw buildException( "forceSession operation fail", null );
    }
    sleep( 1000 );
    var res = null;
    try
    {
        res = db.list( 3, { SessionId: SessionId } ).toArray();
    } catch( e )
    {
        throw buildException( "list Session by sessionId operation fail", null );
    }
    if( res.length !== 0 )
    {
        throw buildException( "current session be forced", null );
    }

    /**3.b*/
    var sessionList = db.list( 3 ).toArray();
    var forceSession = JSON.parse( sessionList[0] );
    var NodeName = forceSession.NodeName;
    var SessionId = forceSession.SessionID;
    var HostName = NodeName.split( ":" )[0];
    var svcname = NodeName.split( ":" )[1];
    var infoByNodeName = new InfoByNodeName( NodeName );
    var GroupName = infoByNodeName.getGroupNameByNodeName();
    try
    {
        db.forceSession( SessionId, { GroupName: GroupName, svcname: svcname, HostName: HostName } );
    } catch( e )
    {
        throw buildException( "forceSession operation fail", null );
    }
    sleep( 1000 );
    var res = null;
    try
    {
        res = db.list( 3, { SessionId: SessionId } ).toArray();
    } catch( e )
    {
        throw buildException( "list Session by sessionId operation fail", null );
    }
    if( res.length !== 0 )
    {
        throw buildException( "current session be forced", null );
    }
}