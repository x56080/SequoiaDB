/******************************************************************************
*@Description : test js object ssh function:
*               getLocalIP() getPeerIP()
*               seqDB-13176:获取本地IP和远程IP
*@author      : Liang XueWang
******************************************************************************/
function testIP ( hostname )
{
   var ssh = newSsh( hostname, sdbUser, sdbPasswd, sshPort );
   var localIp = ssh.getLocalIP();
   var expect = getLocalIPAddr();
   if( localIp !== expect )
   {
      if( localIp === "127.0.0.1" && expect.slice( 0, 4 ) === "127." )
      {
      }
      else
      {
         throw buildException( "testIP", null, "test local ip",
            expect, localIp );
      }
   }
   var peerIp = ssh.getPeerIP();
   expect = getIPAddr( hostname );
   if( peerIp !== expect )
   {
      throw buildException( "testIP", null, "test peer ip",
         expect, peerIp );
   }
   ssh.close();
}

function main ()
{
   var remotehost = toolGetRemotehost();
   println( "ssh hostname: " + remotehost );

   if( !checkSsh( remotehost, sdbUser, sdbPasswd, sshPort ) )
   {
      return;
   }

   testIP( remotehost );
}

main()
