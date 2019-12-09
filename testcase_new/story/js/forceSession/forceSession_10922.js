/**************************************
 * @Description: 测试用例 seqDB-10922 :: 版本: 1 :: 指定sessionID不存在
 * @author: ouyangzhongnan
 * @Date: 2017-01-05
 * @RunDemo:
 * sdb -f "func.js,forceSession_10922.js" -e "var CHANGEDPREFIX='PREFIX';var COORDHOSTNAME='sdbserver01';var COORDSVCNAME='11810'"
 **************************************/

/*
 步骤：
 1、连接coord节点 
 2、执行db.forceSession(sessionID,options)终止会话，其中sessionID在集群中不存在，分别验证如下两种情况： 
 a、sessionID不存在
 b、sessionID存在，指定options不匹配，如hostname、nodeID和sessionID不匹配
 3、查看操作结果
 结果：
 1、操作失败，返回对应错误信息（如指定sessionID不存在等相关错误信息）
 */
// [备注：用例并行跑]

main();
function main ()
{
    if( commIsStandalone( db ) ) return;

    // TODO 2.a: force 一个集群中不存在sessionid
    var sessionId = 1000000000;
    try
    {
        db.forceSession( sessionId );
        throw new Error( 0 );
    } catch( e )
    {
        if( e !== -62 )
        {
            println( "exception is => " + e );
            throw buildException( "TODO 2.a: db.forceSession(sessionId)", e, "force session by session=" + sessionId, "forceSession fail and throw exception=-62", "forceSession success or fail but throw exception!=-62" );
        }
    }

    // TODO 2.b： force 一个集群中存在的sessionid，但是options不存在
    var sessionList = db.list( SDB_LIST_SESSIONS, {
        Status: { $ne: "Waiting" },
        Type: { $nin: ["Agent", "ShardAgent", "CoordAgent", "ReplAgent", "HTTPAgent"] }
    } ).toArray();
    var sessionId = JSON.parse( sessionList[Math.ceil( Math.random() * 1000 ) % sessionList.length] ).SessionID;
    try
    {
        db.forceSession( sessionId, { HostName: "sdbserver01", svcname: "21810" } );
        throw new Error( 0 );
    } catch( e )
    {
        if( e !== -155 )
        {
            println( "exception is => " + e );
            throw buildException( "TODO 2.b: db.forceSession(sessionId)", e, "force session by options and session=" + sessionId, "forceSession fail and throw exception=-155", "forceSession success or fail but throw exception!=-155" );
        }
    }
}