/******************************************************************************
*@Description : test js object System function: runService
*               TestLink : 10668 System对象启动、停止服务如ssh服务
*                          10669 System对象启动服务，启动已启动的服务
*                          10670 System对象停止服务，停止sdbcm服务
*                          10671 System对象停止服务，停止已停止的服务
*@author      : Liang XueWang
******************************************************************************/

// 测试启动停止服务，获取服务状态（SSH服务）
SystemTest.prototype.testRunServiceSSH = function()
{
   this.init() ;
   
   // 检查cm用户是否为root
   var user = this.system.getCurrentUser().toObj().user ;
   if( user !== "root" )
   {
      println( user + " have no permission to run ssh service." ) ;
      return ;
   }
   if( !isSSHExist( this.hostname, this.svcname ) )
      return ;
   
   // 获取服务状态
   var info = this.system.runService( "ssh", "status", "" ) ;
   if( info.indexOf( "start" ) !== -1 )   // 如果服务已启动，则停止再启动服务
   {
      this.system.runService( "ssh", "stop", "" ) ;
      var status = this.system.runService( "ssh", "status" ) ;
      checkStatus( status, "stop" ) ;
      this.system.runService( "ssh", "start", "" ) ;
      status = this.system.runService( "ssh", "status" ) ;
      checkStatus( status, "start" ) ;   
   }
   else if( info.indexOf( "stop" ) !== -1 )  // 如果服务已停止，则启动服务
   {
      this.system.runService( "ssh", "start", "" ) ;
      var status = this.system.runService( "ssh", "status" ) ;
      checkStatus( status, "start" ) ;   
   }
   else
   {
      throw buildException( "testRunService", null, "get service status " + this, 
                            "start/stop", info ) ;
   }
   this.release() ;
}


// 测试重复启动或停止服务，（SSH服务）
SystemTest.prototype.testRunServiceDuplicate = function()
{
   this.init() ;
   
   // 检查cm用户是否为root
   var user = this.system.getCurrentUser().toObj().user ;
   if( user !== "root" )
   {
      println( user + " have no permission to run ssh service." ) ;
      return ;
   }
   if( !isSSHExist( this.hostname, this.svcname ) )
      return ;
   
   // 获取服务状态
   var info = this.system.runService( "ssh", "status" ) ;
   var command ;
   if( info.indexOf( "start" ) !== -1 )
      command = "start" ;
   else
      command = "stop" ;   
   try
   {
      this.system.runService( "ssh", command ) ;
      throw 0 ;
   }
   catch( e )
   {
      if( e !== 1 )
      {
         throw buildException( "testRunServiceDuplicate", e, 
               "run service duplicate " + this, 1, e ) ;
      }
   }
   // 测试完成后，启动服务
   try
   {
      this.system.runService( "ssh", "start" ) ;
   }
   catch( e )
   {
      if( e !== 1 )
      {
         throw buildException( "testRunServiceDuplicate", e, 
               "test start service in the end " + this, 1, e ) ;
      }
   }
   this.release() ;
}

// 测试停止sdbcm服务(手工验证)
SystemTest.prototype.testStopSdbcm = function()
{
   this.init() ;
   
   // 检查cm用户是否为root
   var user = this.system.getCurrentUser().toObj().user ;
   if( user === "root" )
   {
      // println( "Cannot stop sdbcm service owned by root." ) ;
      return ;
   }
   
   if( this.system == System )
   {
      println( "Stop using sdbcm will be stucked." ) ;
      return ;
   }
   try
   {
      this.system.runService( "sdbcm", "stop" ) ;
   }
   catch( e )
   {
      if( e !== -16 )
      {
         throw buildException( "testStopSdbcm", e, "stop sdbcm " + this, -16, e ) ;     
      }
   }
   this.release() ;   
}

/******************************************************************************
*@Description : check service status after stop/start service
*@author      : Liang XueWang            
******************************************************************************/
function checkStatus( status, msg )
{
   if( status.indexOf( msg ) === -1 )
   {
      throw buildException( "checkStatus", null, 
            "check service status " + msg, status, msg ) ;
   }   
}

/******************************************************************************
*@Description : check ssh service exist or not
*@author      : Liang XueWang            
******************************************************************************/
function isSSHExist( hostname, svcname )
{
   var remote = new Remote( hostname, svcname ) ;
   var cmd = remote.getCmd() ;
   var exist ;
   try
   {
      cmd.run( "service ssh status" ) ;
      exist = true ;
   }
   catch( e )
   {
      if( e === 1 )
         exist = false ;
      else
         throw buildException( "isSSHExist", e ) ;
   }
   remote.close() ;
   return exist ;
}

function main()
{
   // 获取本地主机和远程主机
   var localhost = toolGetLocalhost() ;
   var remotehost = toolGetRemotehost() ;
   
   var localSystem = new SystemTest( localhost, CMSVCNAME ) ;
   var remoteSystem = new SystemTest( remotehost, CMSVCNAME ) ;
   var systems = [ localSystem, remoteSystem ] ;
   
   for( var i = 0;i < systems.length;i++ )
   {
      // 测试启动停止ssh服务
      systems[i].testRunServiceSSH() ;
      
      // 测试重复启动停止ssh服务
      systems[i].testRunServiceDuplicate() ;
      
      // 测试停止sdbcm服务
      // systems[i].testStopSdbcm() ;
   }
}

main()