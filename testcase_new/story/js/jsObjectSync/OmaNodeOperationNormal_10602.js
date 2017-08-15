/******************************************************************************
*@Description : test js object oma function: createCoord removeCoord createData
*                              Normal        removeData startNode stopNode 
*               TestLink: 10602 Oma创建、删除、启动、停止协调节点和数据节点
*@author      : Liang XueWang
******************************************************************************/

// 测试正常创建启动停止删除协调节点
OmaTest.prototype.testCoordNodeOperationNormal = function( svcname )
{
   this.testInit() ;
   try
   {
      var dbpath = RSRVNODEDIR + svcname ;
      this.oma.createCoord( svcname, dbpath ) ;
      this.oma.startNode( svcname ) ;
      this.oma.stopNode( svcname ) ;
      this.oma.removeCoord( svcname ) ; 
   }
   catch( e )
   {
      println( "coord " + svcname + " dbpath " + dbpath ) ;
      throw buildException( "testCoordNodeOperationNormal", e, 
                            "coord node operation " + this, 0, e ) ;
   }
   this.oma.close() ;  
}

// 测试正常创建启动停止删除数据节点
OmaTest.prototype.testDataNodeOperationNormal = function( svcname )
{
   this.testInit() ;
   try
   {
      var dbpath = RSRVNODEDIR + svcname ;
      this.oma.createData( svcname, dbpath ) ;
      this.oma.startNode( svcname ) ;
      checkDataNodeValid( this.hostname, svcname ) ;
      this.oma.stopNode( svcname ) ;
      this.oma.removeData( svcname ) ; 
   }
   catch( e )
   {
      println( "data " + svcname + " dbpath " + dbpath ) ;
      throw buildException( "testDataNodeOperationNormal", e,
                            "data node operation " + this, 0, e ) ;
   }
   this.oma.close() ;   
}

/******************************************************************************
*@Description : check node is valid ( create cs cl in node )
*@author      : Liang XueWang              
******************************************************************************/
function checkDataNodeValid( hostname, svcname )
{
   try
   {
      var db = new Sdb( hostname, svcname ) ;
      var CsName = "testDataNodeValidCs" ;
      var cs = db.createCS( CsName ) ;
      cs.createCL( "bar" ) ;
      db.dropCS( CsName ) ;
      db.close() ;
   }
   catch( e )
   {
      println( "node " + hostname + ":" + svcname ) ;
      throw buildException( "checkDataNodeValid", e, 
                            "check node " + hostname + ":" + svcname, 0, e ) ;
   }
}

function main()
{
   // 获取本地主机和远程主机
   var localhost = toolGetLocalhost() ;
   var remotehost = toolGetRemotehost() ;
   
   // 获取本地和远程空闲的端口号
   var svcname1 = toolGetIdleSvcName( localhost["hostname"], CMSVCNAME ) ;
   if( svcname1 === undefined )
   {
      println( "No idle svcname between RSRVPORTBEGIN and RSRVPORTEND local" ) ;
      return ;
   }
   var svcname2 = toolGetIdleSvcName( remotehost["hostname"], CMSVCNAME ) ;
   if( svcname2 === undefined )
   {
      println( "No idle svcname between RSRVPORTBEGIN and RSRVPORTEND remote" ) ;
      return ;
   }
   
   var localOma = new OmaTest( localhost, CMSVCNAME ) ;
   var remoteOma = new OmaTest( remotehost, CMSVCNAME ) ;
   
   var omas = [ localOma, remoteOma ] ;
   var svcnames = [ svcname1, svcname2 ] ;
   
   for( var i = 0;i < omas.length;i++ )
   {
      // 测试协调节点的正常操作
      omas[i].testCoordNodeOperationNormal( svcnames[i] ) ;
      
      // 测试数据节点的正常操作
      omas[i].testDataNodeOperationNormal( svcnames[i] ) ;
   }
}

main()
