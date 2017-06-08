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