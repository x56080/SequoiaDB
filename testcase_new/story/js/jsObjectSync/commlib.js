/******************************************************************************
*@Description : common function for js object System/Oma
*@auhor       : Liang XueWang
******************************************************************************/
function OmaTest( hostName, cmSvcName, isLegalHost, isLegalSvc )
{
   if( hostName === undefined )
      this.hostname = COORDHOSTNAME ;
   else
      this.hostname = hostName ;
   if( cmSvcName === undefined )
      this.svcname = CMSVCNAME ;
   else
      this.svcname = cmSvcName ;
   if( isLegalHost === undefined )
      this.islegalhost = true ;
   else
      this.islegalhost = isLegalHost ;
   if( isLegalSvc === undefined )
      this.islegalsvc = true ;
   else
      this.islegalsvc = isLegalSvc ;
   if( this.islegalhost )
   {
      var db = new Sdb( this.hostname, COORDSVCNAME ) ;
      this.isStandalone = commIsStandalone( db ) ;
      db.close() ;
   }
}

OmaTest.prototype.toString = function()
{
   return ( "OmaTest: hostname=" + this.hostname + " svcname=" + this.svcname ) ;
}

OmaTest.prototype.testInit = function() 
{
   try
   {
      this.oma = new Oma( this.hostname, this.svcname ) ;
   }
   catch( e )
   {
      if( !this.islegalhost && e === -15 )
         ;
      else if( !this.islegalsvc && e === -6 )
         ;
      else
      {
         throw buildException( "testInit", e, "init oma " + this, "0 -15", e ) ;
      }
   } 
}

function SystemTest( hostName, cmSvcName )
{
   if( hostName === undefined )
      this.hostname = COORDHOSTNAME ;
   else
      this.hostname = hostName ;
   if( cmSvcName === undefined )
      this.svcname = CMSVCNAME ;
   else
      this.svcname = cmSvcName ;
   var db = new Sdb( this.hostname, COORDSVCNAME ) ;
   this.isStandalone = commIsStandalone( db ) ;
   db.close() ;
}

SystemTest.prototype.toString = function()
{
   return ( "SystemTest: hostname=" + this.hostname + " svcname=" + this.svcname ) ;
}

SystemTest.prototype.init = function()
{
   if( this.hostname === COORDHOSTNAME || this.hostname === toolGetLocalhost() )
   {
      this.system = System ;
      this.cmd = new Cmd() ;
   }
   else
   {
      this.remote = new Remote( this.hostname, this.svcname ) ;
      this.system = this.remote.getSystem() ;
      this.cmd = this.remote.getCmd() ;
   }
}

SystemTest.prototype.release = function()
{
   if( this.remote !== undefined )
      this.remote.close() ;
}

/******************************************************************************
*@Description : check two number is approximately equal to each other or not
*@author      : Liang XueWang
******************************************************************************/
function isApproEqual( n1, n2 )  // n1 n2 >= 0
{
   var max = n1 > n2 ? n1 : n2 ;
   var min = ( max === n1 ) ? n2 : n1 ;
   return min / max >= 0.85 || max - min <= 2 ;
}

/******************************************************************************
*@Description : get hosts in cluster
*@author      : Liang XueWang            
******************************************************************************/
function toolGetHosts()
{
   var hosts = [] ;
   var k = 0 ;
   
   var db = new Sdb( COORDHOSTNAME, COORDSVCNAME ) ;
   if( commIsStandalone( db ) )
   {
      println( "Run mode is standalone." ) ;
      db.close() ;
      return hosts ;
   }
   
   var tmpInfo = db.listReplicaGroups().toArray() ;
   for( var i = 0;i < tmpInfo.length;i++ )
   {
      var tmpObj = db.eval( "(" + tmpInfo[i] + ")" ).toObj() ;
      var tmpArr = tmpObj.Group ;
      for( var j = 0;j < tmpArr.length;j++ )
      {
         if( hosts.indexOf( tmpArr[j].HostName ) == -1 )
            hosts[k++] = tmpArr[j].HostName ;
      }
   }
   db.close() ;
   return hosts ;
}


/******************************************************************************
*@Description : get local hostname
*@author      : Liang XueWang            
******************************************************************************/
function toolGetLocalhost()
{
   var remote = new Remote( COORDHOSTNAME, CMSVCNAME ) ;
   var cmd = remote.getCmd() ;
   var localhost = cmd.run( "hostname" ).split( "\n" )[0] ;
   remote.close() ;
   return localhost ;
}

/******************************************************************************
*@Description : get a remote hostname in cluster
*               if cluster has no remote host,return localhost
*@author      : Liang XueWang
******************************************************************************/
function toolGetRemotehost()
{
   var hosts = toolGetHosts() ;
   var localhost = toolGetLocalhost() ;
   var remotehost = localhost ;
   for( var i = 0;i < hosts.length;i++ )
   {
      if( hosts[i] !== localhost )
      {
         remotehost = hosts[i] ;
         break ;
      }
   }
   return remotehost ;
}

/******************************************************************************
*@Description : check sdbom exist or not
*@author      : Liang XueWang            
******************************************************************************/
function isOmExist( hostName, cmSvcName )
{
   var oma = new Oma( hostName, cmSvcName ) ;
   var rc ;
   
   var arr = oma.listNodes( { type: "om" } ).toArray() ;
   if( arr.length !== 0 )
      rc = true ;
   else
      rc = false ;
   
   oma.close() ;
   return rc ;
}

/******************************************************************************
*@Description : check getOmaConfigs/getNodeConfigs result
*@author      : Liang XueWang              
******************************************************************************/
function checkResult( info, content, func )
{ 
   for( var i in info )
   {
      var found = false ;
      for( var j = 0;j < content.length;j++ )
      {
         content[j] = content[j].replace( / /g,"" ) ;
         if( content[j][0] === "#" )
            continue ;
         var ind = content[j].indexOf( i ) ;
         if( ind === -1 )
            continue ;
         found = true ;
         var value1 = content[j].slice( ind+i.length+1 ).toLowerCase() ;
         var value2 = info[i].toString().toLowerCase() ;
         if( value1 !== value2 )
            throw buildException( "checkResult", null, func + " i=" + i, value1, value2 ) ;   
      }
      if( found === false )
         throw buildException( "checkResult", func + ", i=" + i ) ;   
   }
}

/******************************************************************************
*@Description : check machine is ppc or not
*@author      : Liang XueWang            
******************************************************************************/
function isPPC( hostName, cmSvcName )
{
   var remote = new Remote( hostName, cmSvcName ) ;
   var cmd = remote.getCmd() ;
   var info ;
   
   info = cmd.run( "uname -m" ).split( "\n" )[0] ;
   
   remote.close() ;
   return ( info.indexOf( "ppc" ) !== -1 ) ;    
}

/******************************************************************************
*@Description : get a idle svcname
*@author      : Liang XueWang            
******************************************************************************/
function toolGetIdleSvcName( hostName, cmSvcName )
{
   var remote = new Remote( hostName, cmSvcName ) ;
   var cmd = remote.getCmd() ;
     
   var svcname ;
   for( svcname = RSRVPORTBEGIN; svcname <= RSRVPORTEND; svcname = svcname*1 + 5 )
   {
      try
      {
         cmd.run( "netstat -anp | grep " + svcname ) ;
      }
      catch( e )
      {
         if( e === 1 )
         {
            remote.close() ;
            return svcname ;
         }
         throw buildException( "toolGetIdleSvcName", e ) ;
      }
   }
   remote.close() ;
   return svcname ;
}

/******************************************************************************
*@Description : get sdbcm user
*@author      : Liang XueWang            
******************************************************************************/
function toolGetSdbcmUser( hostName, cmSvcName )
{
   var remote = new Remote( hostName, cmSvcName ) ;
   var cmd = remote.getCmd() ;
   var command = "ps aux | grep sdbcm | grep -E -v 'grep|sdbcmd' | awk '{print $1}'" ;
   var user = cmd.run( command ).split( "\n" )[0] ;
   return user ;   
}

/******************************************************************************
*@Description : check object is empty or not
*@author      : Liang XueWang            
******************************************************************************/
function isEmptyObject( obj )
{
   for( var k in obj )
      return false ;
   return true ;   
}

/******************************************************************************
*@Description : get sequoiadb dir eg: /opt/sequoiadb /opt/sequoiadb/bin/..
*@author      : Liang XueWang              
******************************************************************************/
function toolGetSequoiadbDir( hostname, svcname )
{
   var dir = [] ;
   var remote = new Remote( hostname, svcname ) ;
   var system = remote.getSystem() ;
   var tmp = system.getEWD() ;
   var ind = tmp.indexOf( "/bin" ) ;
   dir[0] = tmp + "/.." ;
   dir[1] = tmp.slice( 0, ind ) ;
   remote.close() ;
   
   if( hostname === COORDHOSTNAME || hostname === toolGetLocalhost() )
   {
      system = System ;
      tmp = system.getEWD() ;
      ind = tmp.indexOf( "/bin" ) ;
      dir[2] = tmp + "/.." ;
      dir[3] = tmp.slice( 0, ind ) ;
   }
      
   return dir ;
}

/******************************************************************************
*@Description : check user exist or not
*@author      : Liang XueWang              
******************************************************************************/
function isUserExist( hostname, svcname, username )
{
   var remote = new Remote( hostname, svcname ) ;
   var cmd = remote.getCmd() ;
   var exist ;
   try
   {
      cmd.run( "grep '^" + username + ":' /etc/passwd" ) ;
      exist = true ;
   }
   catch( e )
   {
      if( e === 1 )
         exist = false ;
      else
         throw buildException( "isUserExist", e, "check " + username, "1 0", e ) ;
   }
   remote.close() ;
   return exist ;
}

/******************************************************************************
*@Description : check group exist or not
*@author      : Liang XueWang              
******************************************************************************/
function isGroupExist( hostname, svcname, groupname )
{
   var remote = new Remote( hostname, svcname ) ;
   var cmd = remote.getCmd() ;
   var exist ;
   try
   {
      cmd.run( "grep '^" + groupname + ":' /etc/group" ) ;
      exist = true ;
   }
   catch( e )
   {
      if( e === 1 )
         exist = false ;
      else
         throw buildException( "isGroupExist", e, 
                               "check " + groupname, "1 0", e )  ;
   }
   remote.close() ;
   return exist ;
}

/*******************************************************************
* get current time, return time like --> 2017-06-16 17:42:24
*
********************************************************************/
function getCurrentTime()
{
	var date = new Date() ;
    var year = date.getFullYear() ;
    var month = date.getMonth() + 1 ;
    if( month < 10 ) month = "0" + month ;
    var day = date.getDate() ;
    if( day < 10 ) day = "0" + day ;
    var hour = date.getHours() ;
    if( hour < 10 ) hour = "0" + hour ;
    var minute = date.getMinutes() ;
    if( minute < 10 ) minute = "0" + minute ;
    var second = date.getSeconds() ;
    if( second < 10 ) second = "0" + second ;
    var time = year + "-" + month + "-" + day + " " + hour + ":" + minute + ":" + second ;
    return time ;
}