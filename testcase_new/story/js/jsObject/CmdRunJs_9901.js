/******************************************************************************
*@Description : test js object cmd function: runJS
*               TestLink : 9901 执行JS语句 
*@author      : Liang XueWang 
******************************************************************************/

// 测试运行JS命令
CmdTest.prototype.testRunJS = function()
{
   this.init();

   try
   {
      var result = this.cmd.runJS( "1 + 2 * 3" );
   }
   catch( e )
   {
      if( e === -10 && this.isLocal )
         ;
      else
         throw buildException( "testRunJS", e, "run js " + this, 0, e );
   }
   if( ( this.isLocal && result !== undefined ) ||
      ( !this.isLocal && result !== "7" ) )
   {
      throw buildException( "testRunJS", null, "check result " + this,
         "7", result );
   }

   this.release();
}

function main ()
{
   // 获取本地和远程主机
   var localhost = toolGetLocalhost();
   var remotehost = toolGetRemotehost();

   var localCmd = new CmdTest( localhost, CMSVCNAME );
   var remoteCmd = new CmdTest( remotehost, CMSVCNAME );
   var cmds = [localCmd, remoteCmd];

   for( var i = 0; i < cmds.length; i++ )
   {
      // 测试后台运行指令
      cmds[i].testRunJS();
   }
}

main()