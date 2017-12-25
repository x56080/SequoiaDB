/**************************************
 * @Description: 连接coord节点指定sessionID和options参数终止会话  测试用例seqDB-10919
 * @author: ouyangzhongnan
 * @Date: 2017-01-04
 * @RunDemo:
 * sdb -f "func.js,commlib.js,forceSession_10919.js" -e "var CHANGEDPREFIX='PREFIX';var COORDHOSTNAME='sdbserver01';var COORDSVCNAME='11810'"
 **************************************/
/*
 操作步骤：
 1、连接coord节点
 2、通过list()获取当前集群session（如db.list( SDB_LIST_CONTEXTS )）
 3、执行db.forceSession(sessionID,options)终止会话，分别验证如下场景：
 a、options指定NodeID
 b、options指定HostName
 c、options指定svcname
 d、options指定两个参数，如NodeID和hostName
 e、options指定三个参数
 4、查看指定session是否存在
 期望结果：
 1、a、d、e场景，指定session被终止，查看session已不存在
 2、b、c场景，节点上存在指定sessionID被终止，部分节点未匹配到sessionID，系统返回对应错误信息
 [指定session依然存在，2017-01-18改了代码，本来终止当前会话是可以的]
 */

main();

function main() 
{
   if( commIsStandalone( db ) ) 
      return ;
   
   var groups = getDataGroups( db ) ;
   var groupName = groups[0] ;
   var nodes = getGroupNodes( db, groupName ) ;
   var nodeName = nodes[0] ;
   var nodeID = new InfoByNodeName( nodeName ).getNodeIdByNodeName() ;
   var hostName = nodeName.split( ":" )[0] ;
   var svcName = nodeName.split( ":" )[1] ;
   
   var dataDb = new Sdb( hostName, svcName ) ;
   var sessionID = dataDb.list( SDB_LIST_SESSIONS_CURRENT ).next().toObj()["SessionID"] ;
   
   // 3.a forceSession with nodeID
   var option = { NodeID: nodeID } ;
   testForceSession( db, dataDb, sessionID, option ) ;
   
   // 3.b forceSession with  hostName
   dataDb = new Sdb( hostName, svcName ) ;
   sessionID = dataDb.list( SDB_LIST_SESSIONS_CURRENT ).next().toObj()["SessionID"] ;
   option = { HostName: hostName } ;
   var errno = ( getNameNum( nodes, hostName ) === 1 ) ? 0 : -264 ;
   ( errno === -264 ) ? testForceSession( db, dataDb, sessionID, option, errno ) 
                      : testForceSession( db, dataDb, sessionID, option ) ; 
   
   // 3.c forceSession with svcname
   dataDb = new Sdb( hostName, svcName ) ;
   sessionID = dataDb.list( SDB_LIST_SESSIONS_CURRENT ).next().toObj()["SessionID"] ;
   option = { svcname: svcName } ;
   errno = ( getNameNum( nodes, svcName ) === 1 ) ? 0 : -264 ;
   ( errno === -264 ) ? testForceSession( db, dataDb, sessionID, option, errno ) 
                      : testForceSession( db, dataDb, sessionID, option ) ;
   
   // 3.d forceSession with nodeID hostName
   dataDb = new Sdb( hostName, svcName ) ;
   sessionID = dataDb.list( SDB_LIST_SESSIONS_CURRENT ).next().toObj()["SessionID"] ;
   option = { NodeID: nodeID, HostName: hostName } ;
   testForceSession( db, dataDb, sessionID, option ) ;

   // 3.e forceSession with nodeID hostName svcName
   dataDb = new Sdb( hostName, svcName ) ;
   sessionID = dataDb.list( SDB_LIST_SESSIONS_CURRENT ).next().toObj()["SessionID"] ;
   option = { NodeID: nodeID, HostName: hostName, svname: svcName } ;
   testForceSession( db, dataDb, sessionID, option ) ;
}

function testForceSession( db, dataDb, sessionID, option, errno )
{
   try 
   {
      db.forceSession( sessionID, option ) ;
      if( errno !== undefined )
         throw 0 ;
   } 
   catch( e ) 
   {
      var msg = "forceSession " + sessionID + " with option " + JSON.stringify( option ) ;
      if( errno === undefined )
      {
         throw buildException( "testForceSession", e, msg, 0, e ) ;
      }
      else if( e !== errno )
      {
         throw buildException( "testForceSession", e, msg, errno, e ) ;
      }
   }
   try 
   {
      dataDb.list( SDB_LIST_SESSIONS_CURRENT ) ;
      throw 0 ;
   } 
   catch( e ) 
   {
      if( e !== -16 && e !== -15 )
      {
         throw buildException( "testForceSession", e, "check session forced", "-15 -16", e ) ;
      }
   }
}

function getNameNum( nodes, name )
{
   var num = 0 ;
   for( var i = 0;i < nodes.length;i++ )
   {
      if( nodes[i].indexOf( name ) !== -1 )
         num++ ;
   }
   return num ;
}