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
   this.init();

   // 检查cm用户是否为root
   var user = this.system.getCurrentUser().toObj().user;
   if( user !== "root" )
   {
      return;
   }
   if( !isSSHExist( this.hostname, this.svcname ) )
      return;
   // 获取服务状态
   try
   {
      var info = this.system.runService( "ssh", "status", "" );
   }
   catch( e )
   {
      if( e === 3 )
      {
         info = "dead";
      }
   }
   if( info.indexOf( "running" ) !== -1 )   // 如果服务已启动，则停止再启动服务
   {
      var status;
      try
      {
         this.system.runService( "ssh", "stop", "" );
         status = this.system.runService( "ssh", "status" );
         throw new Error( status );
      }
      catch( e )
      {
         if( e.message != 3 )
         {
            throw e;
         }
      }
      finally
      {
         this.system.runService( "ssh", "start", "" );
         status = this.system.runService( "ssh", "status" );
         checkStatus( status, "running" );
      }
   }
   else if( info.indexOf( "dead" ) !== -1 )  // 如果服务已停止，则启动服务
   {
      this.system.runService( "ssh", "start", "" );
      var status = this.system.runService( "ssh", "status" );
      checkStatus( status, "running" );
   }
   else
   {
      throw new Error( "testRunService get service exp status start/stop. but result:" + info );
   }
   this.release();
}

// 测试重复启动或停止服务，（SSH服务）
SystemTest.prototype.testRunServiceDuplicate = function()
{
   this.init();

   // 检查cm用户是否为root
   var user = this.system.getCurrentUser().toObj().user;
   if( user !== "root" )
   {
      return;
   }
   if( !isSSHExist( this.hostname, this.svcname ) )
      return;
   // 获取服务状态
   var info = this.system.runService( "ssh", "status" );
   var command;
   if( info.indexOf( "running" ) !== -1 )
      command = "start";
   else
      command = "stop";
   try
   {
      this.system.runService( "ssh", command );
      throw new Error( "should error" );
   }
   catch( e )
   {
      if( e.message != 0 )
      {
         throw e;
      }
   }
   // 测试完成后，启动服务
   try
   {
      this.system.runService( "ssh", "start" );
      throw new Error( "should error" );
   }
   catch( e )
   {
      if( e.message != 0 )
      {
         throw e;
      }
   }
   this.release();
}

// 测试停止sdbcm服务(手工验证)
SystemTest.prototype.testStopSdbcm = function()
{
   this.init();

   // 检查cm用户是否为root
   var user = this.system.getCurrentUser().toObj().user;
   if( user === "root" )
   {
      return;
   }

   if( this.system == System )
   {
      return;
   }
   try
   {
      this.system.runService( "sdbcm", "stop" );
   }
   catch( e )
   {
      if( e.message != SDB_NETWORK_CLOSE )
      {
         throw e;
      }
   }
   this.release();
}

/******************************************************************************
*@Description : check service status after stop/start service
*@author      : Liang XueWang            
******************************************************************************/
function checkStatus ( status, msg )
{
   if( status.indexOf( msg ) === -1 )
   {
      throw new Error( "testStopSdbcm stop sdbcm" );
   }
}

/******************************************************************************
*@Description : check ssh service exist or not
*@author      : Liang XueWang            
******************************************************************************/
function isSSHExist ( hostname, svcname )
{
   var remote = new Remote( hostname, svcname );
   var cmd = remote.getCmd();
   var exist;
   try
   {
      cmd.run( "service ssh status" );
      exist = true;
   }
   catch( e )
   {
      if( e.message == 3 )
         exist = true;
      else if( e.message == 1 )
         exist = false;
      else
         throw e;
   }
   remote.close();
   return exist;
}

main( test );

function test ()
{

   // 获取本地主机和远程主机
   var localhost = toolGetLocalhost();
   var remotehost = toolGetRemotehost();

   var localSystem = new SystemTest( localhost, CMSVCNAME );
   var remoteSystem = new SystemTest( remotehost, CMSVCNAME );
   var systems = [localSystem, remoteSystem];
   for( var i = 0; i < systems.length; i++ )
   {
      // 测试启动停止ssh服务
      systems[i].testRunServiceSSH();

      // 测试重复启动停止ssh服务
      systems[i].testRunServiceDuplicate();

      // 测试停止sdbcm服务
      // systems[i].testStopSdbcm();
   }
}