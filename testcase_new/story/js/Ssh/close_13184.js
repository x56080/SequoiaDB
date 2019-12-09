/******************************************************************************
*@Description : test js object ssh function:
*               close()
*               seqDB-13184:重复执行close关闭ssh连接
*               seqDB-13185:关闭ssh连接后执行操作
*@author      : Liang XueWang
******************************************************************************/
function testClose ( hostname )
{
   var ssh = newSsh( hostname, sdbUser, sdbPasswd, sshPort );
   ssh.close();

   try
   {
      ssh.getLocalIP();
      ssh.getPeerIP();
      ssh.close();
   }
   catch( e )
   {
      throw buildException( "testClose", e, "test operation after close", 0, e );
   }

   try
   {
      ssh.exec( "hostname" );
      throw 0;
   }
   catch( e )
   {
      if( e !== -15 )
      {
         throw buildException( "testClose", e, "test exec after close", -15, e );
      }
   }

   try
   {
      ssh.getLastRet();
      ssh.getLastOut();
   }
   catch( e )
   {
      throw buildException( "testClose", e, "test operation after close", 0, e );
   }

   try
   {
      ssh.push( "/tmp/src.txt", "/tmp/dst.txt" );
      throw 0;
   }
   catch( e )
   {
      if( e !== -15 )
      {
         throw buildException( "testClose", e, "test push after close", -15, e );
      }
   }

   try
   {
      ssh.pull( "/tmp/src.txt", "/tmp/dst.txt" );
      throw 0;
   }
   catch( e )
   {
      if( e !== -15 )
      {
         throw buildException( "testClose", e, "test pull after close", -15, e );
      }
   }
}

function main ()
{
   var remotehost = toolGetRemotehost();
   println( "ssh hostname: " + remotehost );

   if( !checkSsh( remotehost, sdbUser, sdbPasswd, sshPort ) )
   {
      return;
   }

   testClose( remotehost );
}

main()
