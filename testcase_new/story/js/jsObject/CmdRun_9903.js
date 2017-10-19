/******************************************************************************
*@Description : test js object cmd function: run getCommand getLastRet 
*                                            getLastOut
*               seqDB-9903:执行命令后获取执行命令、执行结果、返回值
*               seqDB-13019:执行无权限的命令
*@author      : Liang XueWang 
******************************************************************************/

// 测试运行正确指令
CmdTest.prototype.testRunNormal = function()
{
   this.init() ;
   
   var result = this.cmd.run( "ls", "/tmp" ) ;
   var command = this.cmd.getCommand() ;   // 获取上次执行的命令
   if( command !== "ls /tmp" )
   {
      throw buildException( "testRun", null, "test getCommand " + this, 
                            "ls /tmp", command ) ;
   }
   var ret = this.cmd.getLastRet() ;       // 获取上次命令是否执行正常
   if( ret !== 0 )
   {
      throw buildException( "testRun", null, "test getLastRet " + this, 
                            "ls /tmp", ret ) ;
   }
   var out = this.cmd.getLastOut() ;       // 获取上次命令执行的返回结果
   if( out !== result )
   {
      throw buildException( "testRun", null, "test getLastOut " + this, 
                            result, out ) ;
   }
   
   this.release() ;
}

// 测试运行错误指令
CmdTest.prototype.testRunAbnormal = function()
{
   this.init() ;
   
   try
   {
      this.cmd.run( "led", "." ) ;
   }
   catch( e )
   {
      if( e !== 127 )
         throw buildException( "testRunAbnormal", e, 
                               "run abnormal command " + this, 127, e ) ;
   }
   var command = this.cmd.getCommand() ;
   if( command !== "led ." )
   {
      throw buildException( "testRunAbnormal", null, "test getCommand " + this, 
                            "led .", command ) ;
   }
   var ret = this.cmd.getLastRet() ;
   if( ret !== 127 )
   {
      throw buildException( "testRunAbnormal", null, "test getLastRet " + this, 
                            127, ret ) ;
   }
   var out = this.cmd.getLastOut() ;
   if( out.indexOf( "not found" ) === -1 &&
       out.indexOf( "未找到命令") === -1 )
   {
      throw buildException( "testRunAbnormal", null, "test getLastOut " + this, 
                            "not found", out ) ;
   }
   
   this.release() ;
}

// 测试运行无权限的命令useradd
CmdTest.prototype.testRunNoPermission = function()
{
    this.init() ;
    
    var user = this.cmd.run( "whoami" ).split( "\n" )[0] ;
    if( user === "root" )
    {
        println( "cmd user is root" ) ;
        this.release() ;
        return ;
    }
    try
    {
        this.cmd.run( "useradd liangxw" ) ;
        throw 0 ;
    }
    catch( e )
    {
        if( e !== 1 )
        {
            throw buildException( "testRunNoPermission", null, 
                  "test run useradd with user " + user, 1, e ) ;
        }
    }
    
    this.release() ;
}

function main()
{
   // 获取本地和远程主机
   var localhost = toolGetLocalhost() ;
   var remotehost = toolGetRemotehost() ;
   
   var localCmd = new CmdTest( localhost, CMSVCNAME ) ;
   var remoteCmd = new CmdTest( remotehost, CMSVCNAME ) ;
   var cmds = [ localCmd, remoteCmd ] ;
   
   for( var i = 0;i < cmds.length;i++ )
   {
      // 测试运行指令
      cmds[i].testRunNormal() ;
      cmds[i].testRunAbnormal() ;
      cmds[i].testRunNoPermission() ;
   }
}

main()