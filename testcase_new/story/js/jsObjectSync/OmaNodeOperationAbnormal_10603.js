/******************************************************************************
*@Description : test js object oma function: createCoord removeCoord createData
*                              Abnormal      removeData startNode stopNode close             
*               TestLink: 10603 Oma创建节点，端口已存在
*                         10604 Oma创建节点，文件路径无权限                 
*                         10607 Oma创建节点，节点配置信息错误
*                         10608 Oma创建节点，添加不存在的配置项
*                         10609 Oma删除节点，端口号不存在
*                         10610 Oma删除节点，端口号类型不匹配
*                         10611 Oma删除节点，节点配置信息不正确
*                         10614 Oma启动节点，节点不存在 
*                         10615 Oma停止节点，节点不存在
*                         10616 关闭Oma对象
*@author      : Liang XueWang               
******************************************************************************/

// 测试创建已存在的节点
OmaTest.prototype.testCreateExistCoord = function()
{
   this.testInit() ;
   
   try
   {
      var svcname = COORDSVCNAME ;
      var dbpath = RSRVNODEDIR + svcname ;
      this.oma.createCoord( svcname, dbpath ) ;
      throw 0 ;
   }
   catch( e )
   {
      if( e !== -145 )
      {
         println( "create coord " + svcname + " dbpath " + dbpath) ;
         throw buildException( "testCreateExistCoord", e, 
                               "create exist coord " + this, -145, e ) ;
      }
   }
   
   this.oma.close() ;
}

// 测试删除不存在的节点
OmaTest.prototype.testRemoveNotExistCoord = function( svcname )
{
   this.testInit() ;
   try
   {
      this.oma.removeCoord( svcname ) ;
      throw 0 ;
   }
   catch( e )
   {
      if( e !== -146 )
      {
         println( "remove coord " + svcname ) ;
         throw buildException( "testRemoveNotExistCoord", e, 
                               "remove not exist coord " + this, -146, e ) ;
      }
   }
   this.oma.close() ;
}

// 测试删除节点时使用错误端口
OmaTest.prototype.testRemoveCoordWithWrongSvc = function()
{
   this.testInit() ;
   try
   {
      this.oma.removeCoord( CMSVCNAME ) ;
      throw 0 ;
   }
   catch( e )
   {
      if( e !== -146 )
      {
         println( "remove coord " + CMSVCNAME ) ;
         throw buildException( "testRemoveCoordWithWrongSvc", e, 
                               "remove coord with cmsvcname " + this, -146, e ) ;
      }
   }
   this.oma.close() ;
}

// 测试删除节点时使用错误配置项
OmaTest.prototype.testRemoveCoordWithWrongConf = function()
{
   this.testInit() ;
   
   if( this.isStandalone )
   {
      // println( "Run mode is standalone" ) ;
      return ;
   }
   
   try
   {
      this.oma.removeCoord( COORDSVCNAME, { clustername: "!@#$%^&*" } ) ;
      throw 0 ;
   }
   catch( e )
   {
      if( e !== -146  )
      {
         println( "remove coord " + COORDSVCNAME ) ;
         throw buildException( "testRemoveCoordWithWrongConf", e, 
                               "remove coord with wrong config " + this, -146, e ) ;
      }
   }
   this.oma.close() ;
}

// 测试启动不存在的节点
OmaTest.prototype.testStartNotExistNode = function( svcname )
{
   this.testInit() ;
   try
   {
      this.oma.startNode( svcname ) ;
      throw 0 ;
   }
   catch( e )
   {
      if( e !== -146 )
      {
         println( "start node " + svcname ) ;
         throw buildException( "testStartNotExistNode", e, 
                               "start not exist node " + this, -146, e ) ;
      }
   }
   this.oma.close() ;
}

// 测试停止不存在的节点
OmaTest.prototype.testStopNotExistNode = function( svcname )
{
   this.testInit() ;
   try
   {
      this.oma.stopNode( svcname ) ;
   }
   catch( e )
   {
      println( "stop node " + svcname ) ;
      throw buildException( "testStopNotExistNode", e, 
                            "stop not exist node " + this, 0, e ) ;
   }
   this.oma.close() ;
}

// 测试创建节点时使用错误配置项
OmaTest.prototype.testCreateCoordWithWrongConf = function( svcname )
{
   this.testInit() ;
   try
   {
      var dbpath = RSRVNODEDIR + svcname ;
      // 不正确的配置项 role: om 配置文件自动修改为 role: coord
      this.oma.createCoord( svcname, dbpath, { role: "om" } ) ;
      this.oma.startNode( svcname ) ;
      this.oma.stopNode( svcname ) ;
      this.oma.removeCoord( svcname ) ;
      // 不存在的配置项 abc: 123 写入配置文件
      this.oma.createCoord( svcname, dbpath, { abc: "123" } ) ;
      this.oma.startNode( svcname ) ;
      this.oma.stopNode( svcname ) ;
      this.oma.removeCoord( svcname ) ;
   }
   catch( e )
   {
      println( "coord " + svcname + " dbpath " + dbpath ) ;
      throw buildException( "testCreateCoordWithWrongConf", e, 
                            "create coord with wrong config " + this, 0, e ) ;
   }
   this.oma.close() ;
}

// 测试创建节点时节点目录无权限
OmaTest.prototype.testCreateCoordWithNoPermit = function( svcname )
{
   this.testInit() ;
   
   var remote = new Remote( this.hostname, this.svcname ) ;
   var system = remote.getSystem() ;
   user = system.getCurrentUser().toObj().user ;
   if( user === "root" ) return ;
   
   // make no permission dir
   var file = remote.getFile() ;
   var dirName = "/tmp/noPerDir/" ;
   file.mkdir( dirName ) ;
   file.chmod( dirName, 0000 ) ;
   
   try
   {
      this.oma.createCoord( svcname, dirName + svcname ) ;
      throw 0 ;
   }
   catch( e )
   {
      if( e !== -3 )
      {
         println( "create coord " + svcname + " dbpath " + dirName + svcname ) ;
         throw buildException( "testCreateCoordWithNoPermit", e, 
                               "create coord with no permit " + this, -3, e ) ;
      }
   }
   
   file.chmod( dirName, 0755 ) ;
   file.remove( dirName ) ;
   remote.close() ;
   this.oma.close() ;
}

// 测试oma关闭后执行操作
OmaTest.prototype.testOmaClose = function()
{
   this.testInit() ;
   this.oma.close() ;
   try
   {
      this.oma.stopNode( COORDSVCNAME ) ;
      throw 0 ;
   }
   catch( e )
   {
      if( e !== -6 )
      {
         println( "stop node " + COORDSVCNAME ) ;
         throw buildException( "testOmaClose", e, 
                               "stop coord node after close " + this, -6, e ) ;
      }
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
   
   for( i = 0;i < omas.length;i++ )
   {
      // 测试创建已存在的节点
      omas[i].testCreateExistCoord() ;
      
      // 测试删除不存在的节点
      omas[i].testRemoveNotExistCoord( svcnames[i] ) ;
      
      // 测试删除节点时端口号不匹配
      omas[i].testRemoveCoordWithWrongSvc() ;
      
      // 测试删除节点时配置项非法
      omas[i].testRemoveCoordWithWrongConf() ;
      
      // 测试启动不存在的节点
      omas[i].testStartNotExistNode( svcnames[i] ) ;
      
      // 测试停止不存在的节点
      omas[i].testStopNotExistNode( svcnames[i] ) ;
      
      // 测试创建节点时配置项非法
      omas[i].testCreateCoordWithWrongConf( svcnames[i] ) ;
      
      // 测试创建节点时路径无权限
      omas[i].testCreateCoordWithNoPermit( svcnames[i] ) ;
      
      // 测试oma关闭后执行操作
      omas[i].testOmaClose() ;
   }
}

main()
