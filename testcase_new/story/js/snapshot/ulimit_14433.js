/*******************************************************************
* @Description : test health snapshot field "Ulimit"
*                seqDB-14433:Ulimit字段测试
* @author      : linsuqiang
* 
*******************************************************************/
try
{
   main();
}
catch(e)
{
   if(e.constructor === Error)
   {
      println(e.stack);
   }
   throw e;
}

function main()
{
   var remote = new Remote( COORDHOSTNAME, 11790 );
   var cmd = remote.getCmd();
   var nodeInfo = getLocalNodeInfo( cmd );
   var expUlimit = getUlimitByCmd( cmd, nodeInfo.PID );
   var actUlimit = getUlimitBySnap( db, nodeInfo.GID, nodeInfo.NID );
   isEquals( expUlimit, actUlimit );
}

function getUlimitByCmd( cmd, PID )
{
   try
   {
      var ulimitJson = {};
      ulimitJson.CoreFileSize    = getUlimitItem( cmd, PID, "Max core file size" );
      ulimitJson.VirtualMemory   = getUlimitItem( cmd, PID, "Max address space" );
      ulimitJson.OpenFiles       = getUlimitItem( cmd, PID, "Max open files" );
      ulimitJson.NumProc         = getUlimitItem( cmd, PID, "Max processes" );
      ulimitJson.FileSize        = getUlimitItem( cmd, PID, "Max file size" );
      return ulimitJson;
   }
   catch( e )
   {
      throw new Error( e );
   }
}

function getUlimitItem( cmd, PID, itemName )
{
   try
   {
      var result = cmd.run( "cat /proc/" + PID + "/limits | " + 
            "grep '" + itemName + "' | " + 
            "awk -F '  +' '{print $2}'" ); 
      result = result.slice( 0, result.length - 1 );
      if( result === "unlimited" )
      {
         result = -1;
      }
      result = parseInt( result );
      return result;
   }
   catch( e )
   {
      throw new Error( e );
   }
}

function getUlimitBySnap( db, GID, NID )
{
   try
   {
      var cond = {};
      if( GID != '-' ) // '-' means standalone
      {
         GID = parseInt( GID );
         NID = parseInt( NID );
         cond.NodeID = { $et: [ GID, NID ] };
      }

      var cur = db.snapshot( SDB_SNAP_HEALTH, cond );
      var result = cur.next().toObj().Ulimit;
      cur.close();
      return result;
   }
   catch(e)
   {
      throw new Error( e );
   }
}

function getLocalNodeInfo( cmd )
{
   try
   {
      var installPath = commGetInstallPath();
      var infoStr = cmd.run( installPath + "/bin/sdblist -l | grep sequoiadb | head -n 1" );
      if( infoStr == "" )
      {
         throw "no any node of localhost";
      }
      var infoArr = infoStr.split( /\s+/ );
      var infoJson = {};
      infoJson.Name        = infoArr[0];
      infoJson.SvcName     = infoArr[1];
      infoJson.Role        = infoArr[2];
      infoJson.PID         = infoArr[3];
      infoJson.GID         = infoArr[4];
      infoJson.NID         = infoArr[5];
      infoJson.PRY         = infoArr[6];
      infoJson.GroupName   = infoArr[7];
      infoJson.StartTime   = infoArr[8];
      infoJson.DBPath      = infoArr[9];
      return infoJson;
   }
   catch( e )
   {
      throw new Error( e );
   }
}

function isEquals( a, b )
{
   try
   {
      if( a instanceof Object && b instanceof Object )
      {
         var aProps = Object.getOwnPropertyNames( a );
         var bProps = Object.getOwnPropertyNames( b );

         if( aProps.length != bProps.length)
         {
            return "aProps.length is " + aProps.length + ", bProps.length " + bProps.length;
         }

         for( var i = 0; i < aProps.length; ++i )
         {
            var propName = aProps[i];
            if( a[propName] !== b[propName] )
            {
               throw "a[propName] is " + a[propName] + ", b[propName] is " + b[propName];
            }
         }
      }
      else
      {
         throw "typeof a is " + typeof a + ", typeof b is " + typeof b;
      }
   }
   catch( e )
   {
      throw new Error( e );
   }
}

