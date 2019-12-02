/******************************************************************************
*@Description : test js object ssh function:
*               var ssh = newSsh( hostname, [user], [password], [port] )
*               seqDB-13171:使用ssh连接未建立信赖关系的用户( 不指定密码 )
*               seqDB-13172:使用ssh连接主机用户，用户密码错误
*               seqDB-13173:使用ssh连接主机用户，端口错误
*               seqDB-13174:不指定用户、用户密码、端口（使用默认值）创建ssh
*               seqDB-13175:指定用户、用户密码、端口创建ssh
*@author      : Liang XueWang
******************************************************************************/
function testSshWithoutPasswd( hostname )
{
   try
   {
      var ssh = newSsh( hostname, sdbUser, undefined, undefined, -6 ); 
      ssh.close(); 
   }
   catch( e )
   {
      if( e !== -6 )
      {
         throw buildException( "testSshWithoutPasswd", e, 
         "test ssh with " + sdbUser + ", no passwd", "0 -6", e ); 
      }
   }
}

function testSshWithIllegalPara( hostname )
{
   try
   {
      var ssh = newSsh( hostname, sdbUser, "ssss", undefined, -6 ); 
      throw 0; 
   }
   catch( e )
   {
      if( e !== -6 )
      {
         throw buildException( "testSshWithIllegalPara", e, 
         "test ssh with wrong passwd", -6, e ); 
      }
   }
   try
   {
      var ssh = newSsh( hostname, sdbUser, sdbPasswd, 24, -79 ); 
      throw 0; 
   }
   catch( e )
   {
      if( e !== -79 )
      {
         throw buildException( "testSshWithIllegalPara", e, 
         "test ssh with wrong port", -79, e ); 
      }
   }
}

function testSshWithDefault( hostname )
{
   try
   {
      var ssh = newSsh( hostname ); 
      ssh.close(); 
   }
   catch( e )
   {
      throw buildException( "testSshWithDefault", e, 
      "test ssh with default para", 0, e ); 
   }
}

function testSshNormal( hostname )
{
   try
   {
      var ssh = newSsh( hostname, sdbUser, sdbPasswd, sshPort ); 
      ssh.close(); 
   }
   catch( e )
   {
      throw buildException( "testSshNormal", e, "test ssh", 0, e ); 
   }
}

function main()
{
   var remotehost = toolGetRemotehost(); 
   println( "ssh hostname: " + remotehost ); 
   
   if( !checkSsh( remotehost, sdbUser, sdbPasswd, sshPort ) )
   {
      return; 
   }
   
   testSshWithoutPasswd( remotehost ); 
   testSshWithIllegalPara( remotehost ); 
   // testSshWithDefault( remotehost ); 
   testSshNormal( remotehost ); 
}

main()
