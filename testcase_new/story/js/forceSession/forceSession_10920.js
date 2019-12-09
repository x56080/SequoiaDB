/**************************************
 * @Description: 非coord节点指定options参数终止会话  测试用例 seqDB-10920
 * @author: ouyangzhongnan
 * @Date: 2017-01-04
 * @RunDemo:
 * sdb -f "func.js,commlib.js,forceSession_10920.js" -e "var CHANGEDPREFIX='PREFIX';var COORDHOSTNAME='sdbserver01';var COORDSVCNAME='11810'"
 **************************************/
/*
 步骤：
 1、分别连接data节点、catalog节点
 2、通过list()获取当前集群session（如list（3））
 3、执行db.forceSession(sessionID,options)终止会话，其中options分别配置正确和不匹配的参数，如hostname/nodeID
 4、查看指定session是否存在
 结果：
 1、指定session被终止，查看session已不存在
 2、options参数配置失效，配置错误不影响强杀指定session
 */

main();
function main ()
{
    if( commIsStandalone( db ) ) return;
    /**
     * 初始变量值
     *    保存内容, 例：coord = [sdbserver01:11810,sdbserver02:11810,sdbserver03:11810];
     */
    var catalog = [];
    var datagroup = [];
    var SYSCatalogGroup = commGetGroups( db, true, "SYSCatalogGroup", false, true, true );
    for( var i = 1; i < SYSCatalogGroup[0].length; i++ )
    {
        var url = SYSCatalogGroup[0][i].HostName + ":" + SYSCatalogGroup[0][i].svcname;
        catalog.push( url );
    }
    var DataGroup = commGetGroups( db, true, "", true, true, true );
    for( var i = 1; i < DataGroup[0].length; i++ )
    {
        var url = DataGroup[0][i].HostName + ":" + DataGroup[0][i].svcname;
        datagroup.push( url );
    }

    // 连catalog，配置正确options
    doTest( catalog[Math.ceil( Math.random() * 10 ) % catalog.length], true );
    // 连catalog，配置不匹配options
    doTest( catalog[Math.ceil( Math.random() * 10 ) % catalog.length], false );
    // 连data, 配置正确options
    doTest( datagroup[Math.ceil( Math.random() * 10 ) % datagroup.length], true );
    // 连data, 配置不匹配options
    doTest( datagroup[Math.ceil( Math.random() * 10 ) % datagroup.length], false );
}


/**
 * 通过给定的url连接到对应的节点，执行测试用例中指定的操作步骤
 * @param  url 要连接的节点的地址，如sdbserver01:11820
 * @param  flag 配置options参数是否正确， true为正确，false为不匹配
 */
function doTest ( url, flag )
{
    // 建立连接，list(3)获取当前session，并取得对应的sessionId用于force
    var db = getConn( url );
    var currentSession = db.list( SDB_LIST_SESSIONS_CURRENT ).next().toObj();
    var oldSessionID = currentSession.SessionID;
    // force session
    var options = null;
    if( flag )
    {
        options = { "NodeName": currentSession.NodeName }
    } else
    {
        options = { "NodeID": 4321 };
    }
    try
    {
        db.forceSession( oldSessionID, options );
    } catch( e )
    {
        throw buildException( "TODO db.forceSession(oldSessionID, options) fail, options is +" + JSON.stringify( options ), e );
    }
    // 通过oldSessionID查询会话列表，观察oldSessionID是否还存在
    var res = db.list( SDB_LIST_SESSIONS, { "SessionID": oldSessionID } ).toArray();
    if( res.length === 0 )
    {
        throw buildException( "the current session be forced", null, "list Session by SessionID=oldSessionID", "can find the session", "can not find the session" );
    }
}

