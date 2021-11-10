/*****************************************************************************
@Description : seqDB-24556:创建用户，创建SecureSdb对象指定用户密码，使用SecureSdb对象获取节点的连接
@Author      : xiaozhenfan
@CreateTime  : 2021.11.4
@LastEditTime: 2021.11.4
@LastEditors : xiaozhenfan
******************************************************************************/
testConf.skipStandAlone = true;
main(test);

//获取主机名
function getHostName( hostAddr )
{
   var cmd = new Cmd();
   if ( hostAddr === "localhost" )
   {
      var hostName = cmd.run("hostname").split("\n")[0] ;
   }
   else
   {
      var out = cmd.run("grep " + hostAddr + " /etc/hosts|awk '{print $2}'").split("\n")[0];
   }
   return hostName ;
}


function test ( )
{
   var hostName = getHostName( COORDHOSTNAME ) ;
   var nodeName = hostName + ":" + COORDSVCNAME ;
   var isUseSsl = db.snapshot(SDB_SNAP_CONFIGS,{ NodeName: nodeName }, 
                 { usessl:"" }).next().toObj().usessl ;
   if( isUseSsl === "FALSE" )
   {
      db.updateConf( { usessl:true }, { NodeName: nodeName } ) ;
   }

   var userName = "sdbadmin" ;
   var passWrod = "sdbadmin" ;
   var isUsrExist = false ;

   db = new SecureSdb( COORDHOSTNAME, COORDSVCNAME, userName, passWrod )
   try
   {
      db.createUsr( userName, passWrod ) ;
      isUsrExist = true ;
      var db1 = new SecureSdb( COORDHOSTNAME, COORDSVCNAME, userName, passWrod )
      var RGName = "SYSCatalogGroup" ;
      db1.getRG( RGName ).getMaster().connect() ;
   }
   catch(e)
   {
      throw e ;
   }
   finally
   {
      if ( isUseSsl === "FALSE" )
      {
         db.updateConf( { usessl:false }, {NodeName: nodeName } ) ;
      }
      if ( isUsrExist )
      {
         db.dropUsr( userName, passWrod );
      }
   }
}
