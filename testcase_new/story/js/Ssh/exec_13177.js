/******************************************************************************
*@Description : test js object ssh function:
*               exec( command ) getLastRet() getLastOut()
*               seqDB-13177:使用ssh执行正常命令，获取输出结果
*               seqDB-13178:使用ssh执行异常命令（不存在命令），获取输出结果
*@author      : Liang XueWang
******************************************************************************/
function testExecNormal( hostname )
{
   var ssh = newSsh( hostname, sdbUser, sdbPasswd, sshPort ); 
   var host = ssh.exec( "hostname" ).split( "\n" )[0]; 
   if( host !== hostname )
   {
      throw buildException( "testExecNormal", null, 
      "test exec hostname", hostname, host ); 
   }
   var ret = ssh.getLastRet(); 
   if( ret !== 0 )
   {
      throw buildException( "testExecNormal", null, 
      "test getLastRet", 0, ret ); 
   }
   var out = ssh.getLastOut(); 
   if( out.split( "\n" )[0] !== hostname )
   {
      throw buildException( "testExecNormal", null, 
      "test getLastOut", hostname, out ); 
   }
   ssh.close(); 
}

function testExecAbnormal( hostname )
{
   var ssh = newSsh( hostname, sdbUser, sdbPasswd, sshPort ); 
   try
   {
      ssh.exec( "led" ); 
      throw 0; 
   }
   catch( e )
   {
      if( e !== 127 )
      {
         throw buildException( "testExecAbnormal", e, 
         "test exec led", 127, e ); 
      }
   }
   var ret = ssh.getLastRet(); 
   if( ret !== 127 )
   {
      throw buildException( "testExecAbnormal", null, 
      "test getLastRet", 127, e ); 
   }
   var out = ssh.getLastOut(); 
   if( out.indexOf( "not found" )=== -1 &&
   out.indexOf( "未找到命令" )=== -1 )
   {
      throw buildException( "testExecAbnormal", null, 
      "test getLastOut", "command not found", out ); 
   }
   ssh.close(); 
}

function main()
{
   var remotehost = toolGetRemotehost(); 
   println( "ssh hostname: " + remotehost ); 
   
   if( !checkSsh( remotehost, sdbUser, sdbPasswd, sshPort ) )
   {
      return; 
   }
   
   testExecNormal( remotehost ); 
   testExecAbnormal( remotehost ); 
}

main()
