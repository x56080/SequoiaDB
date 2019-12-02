/******************************************************************************
*@Description : common function for js object Ssh
*@auhor       : Liang XueWang
******************************************************************************/

var sdbUser = "sdbadmin"; 
var sdbPasswd = "sdbadmin"; 
var sshPort = 22; 

/******************************************************************************
*@Description : get hosts in cluster
*@author      : Liang XueWang
******************************************************************************/
function toolGetHosts()
{
   var hosts = []; 
   var k = 0; 
   
   var db = new Sdb( COORDHOSTNAME, COORDSVCNAME ); 
   if( commIsStandalone( db ) )
   {
      println( "Run mode is standalone." ); 
      db.close(); 
      return hosts; 
   }
   
   var tmpInfo = db.listReplicaGroups().toArray(); 
   for( var i = 0; i < tmpInfo.length; i++ )
   {
      var tmpObj = JSON.parse( tmpInfo[i] ); 
      var tmpArr = tmpObj.Group; 
      for( var j = 0; j < tmpArr.length; j++ )
      {
         if( hosts.indexOf( tmpArr[j].HostName )== -1 )
         hosts[k++] = tmpArr[j].HostName; 
      }
   }
   db.close(); 
   return hosts; 
}

/******************************************************************************
*@Description : get local hostname( COORDHOSTNAME )
*               localhost means cluster local host, host of COORDHOSTNAME
*@author      : Liang XueWang
******************************************************************************/
function toolGetLocalhost()
{
   // get local host of cluster, with COORDHOSTNAME
   var remote = new Remote( COORDHOSTNAME, CMSVCNAME ); 
   var cmd = remote.getCmd(); 
   var localhost = cmd.run( "hostname" ).split( "\n" )[0]; 
   remote.close(); 
   
   return localhost; 
}

/******************************************************************************
*@Description : get a remote hostname in cluster
*               if cluster has no remote host, return localhost
*@author      : Liang XueWang
******************************************************************************/
function toolGetRemotehost()
{
   var hosts = toolGetHosts(); 
   var localhost = toolGetLocalhost(); 
   var remotehost = localhost; 
   for( var i = 0; i < hosts.length; i++ )
   {
      if( hosts[i] !== localhost )
      {
         remotehost = hosts[i]; 
         break; 
      }
   }
   
   return remotehost; 
}

/******************************************************************************
*@Description : check ssh with user passwd port in host
*@author      : Liang XueWang
******************************************************************************/
function checkSsh( hostname, user, passwd, port )
{
   try
   {
      var ssh = newSsh( hostname, user, passwd, port ); 
      ssh.close(); 
      return true; 
   }
   catch( e )
   {
      println( "ssh with " + user + " " + passwd + " " + port + 
      " failed, e = " + e ); 
      return false; 
   }
}

/******************************************************************************
*@Description : check cm user
*@author      : Liang XueWang
******************************************************************************/
function checkCmUser( hostname, user )
{
   try
   {
      var remote = new Remote( hostname, CMSVCNAME ); 
      var system = remote.getSystem(); 
      var actual = system.getCurrentUser().toObj().user; 
      remote.close(); 
      if( user !== actual )
      {
         println( "cm user is " + actual + ", not " + user ); 
         return false; 
      }
      return true; 
   }
   catch( e )
   {
      throw buildException( "checkCmUser", e, "check cm user " + hostname, 0, e ); 
   }
}

/******************************************************************************
*@Description : get ip address of hostname
*@author      : Liang XueWang
******************************************************************************/
function getIPAddr( hostname )
{
   try
   {
      var cmd = new Cmd(); 
      var command = "cat /etc/hosts | grep " + hostname + " | awk '{print $1}'"; 
      var ip = cmd.run( command ).split( "\n" )[0]; 
      return ip; 
   }
   catch( e )
   {
      throw buildException( "getIPAddr", e, 
      "get ip address of " + hostname, 0, e ); 
   }
}

/******************************************************************************
*@Description : get ip address of local host
*@author      : Liang XueWang
******************************************************************************/
function getLocalIPAddr()
{
   try
   {
      var cmd = new Cmd(); 
      var localhost = cmd.run( "hostname" ).split( "\n" )[0]; 
      var command = "cat /etc/hosts | grep " + localhost + " | awk '{print $1}'"; 
      var ip = cmd.run( command ).split( "\n" )[0]; 
      return ip; 
   }
   catch( e )
   {
      throw buildException( "getLocalIPAddr", e, "get ip of " + localhost, 0, e ); 
   }
}

/******************************************************************************
*@Description : force remove local file
*@author      : Liang XueWang
******************************************************************************/
function rmLocalFile( filename )
{
   try
   {
      var cmd = new Cmd(); 
      cmd.run( "rm -rf " + filename ); 
   }
   catch( e )
   {
      throw buildException( "rmLocalFile", e, "force rm file " + filename, 0, e ); 
   }
}

/******************************************************************************
*@Description : force remove remote file
*@author      : Liang XueWang
******************************************************************************/
function rmRemoteFile( hostname, filename )
{
   try
   {
      var remote = new Remote( hostname, CMSVCNAME ); 
      var cmd = remote.getCmd(); 
      cmd.run( "rm -rf " + filename ); 
      remote.close(); 
   }
   catch( e )
   {
      throw buildException( "rmRemoteFile", e, 
      "force rm file " + filename + " " + hostname, 0, e ); 
   }
}

/******************************************************************************
*@Description : check remote file mode and content
*@author      : Liang XueWang
******************************************************************************/
function checkRemoteFile( hostname, filename, mode, content )
{
   var remote = new Remote( hostname, CMSVCNAME ); 
   var file = remote.getFile( filename ); 
   var actual = file.stat( filename ).toObj().mode; 
   if( mode !== actual )
   {
      throw buildException( "checkRemoteFile", null, "check file mode: " + filename + 
      " " + hostname, mode, actual ); 
   }
   actual = file.read().split( "\n" )[0]; 
   if( content !== actual )
   {
      throw buildException( "checkRemoteFile", null, "check file content: " + filename + 
      " " + hostname, content, actual ); 
   }
   file.close(); 
   remote.close(); 
}

/******************************************************************************
*@Description : check local file mode and content
*@author      : Liang XueWang
******************************************************************************/
function checkLocalFile( filename, mode, content )
{
   var file = new File( filename ); 
   var actual = file.stat( filename ).toObj().mode; 
   if( actual !== mode )
   {
      throw buildException( "checkLocalFile", null, "check file mode: " + filename, 
      mode, actual ); 
   }
   actual = file.read().split( "\n" )[0]; 
   if( actual !== content )
   {
      throw buildException( "checkLocalFile", null, "check file content: " + filename, 
      content, actual ); 
   }
   file.close(); 
}


/******************************************************************************
*@Description : ssh at least three times
*@author      : luweikang
******************************************************************************/
function newSsh( hostname, sdbUser, sdbPasswd, sshPort, error )
{
   var sshSuccess = false; 
   for( var i = 0; i < 3; i++ )
   {
      
      try
      {
         if( hostname == undefined )
         {
            var ssh = new Ssh(); 
         }
         else if( sdbUser == undefined )
         {
            var ssh = new Ssh( hostname ); 
         }
         else if( sdbPasswd == undefined )
         {
            var ssh = new Ssh( hostname, sdbUser ); 
         }
         else if( sshPort == undefined )
         {
            var ssh = new Ssh( hostname, sdbUser, sdbPasswd ); 
         }
         else
         {
            var ssh = new Ssh( hostname, sdbUser, sdbPasswd, sshPort ); 
         }
         sshSuccess = true; 
         break; 
      }
      catch( e )
      {
         if( e == error )
         {
            throw e; 
         }
      }
   }
   if( sshSuccess === false )
   {
      throw buildException( "newSsh()", null, "three retries failed", "ssh success", "ssh failed" ); 
   }
   return ssh; 
}
