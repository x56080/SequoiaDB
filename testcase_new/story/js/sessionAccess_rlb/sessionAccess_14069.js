/* *****************************************************************************
@discretion: createNode,check parameter instatceid
@author：2018-11-20 wangkexin
***************************************************************************** */
import( "../sessionAccess/commlib.js" );
main();

function main ()
{
   try
   {
      var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
      var groupName = "group14069";
      var rg = db.createRG( groupName );
      // effective parameters
      var instanceidList = [12, 0, 1, 255, "123"];
      //invalid parameters
      var errInstanceidList = [12.234, -1, 256, "0x10"];

      //createNode and set instanceid
      println( "begin to createNode and set instanceid" );
      var nodeHostName = db.listReplicaGroups().current().toObj().Group[0].HostName;
      var failedCount = 0;
      for( var i = 0; i < instanceidList.length; i++ )
      {
         var svc = parseInt( RSRVPORTBEGIN ) + 10 * ( i + failedCount );
         var dbpath = RSRVNODEDIR + "data/" + svc;
         var checkSucc = false;
         var times = 0;
         var maxRetryTimes = 10;
         var config = { instanceid: instanceidList[i], diaglevel: 5 };

         do
         {
            try
            {
               rg.createNode( nodeHostName, svc, dbpath, config );
               println( "create node " + nodeHostName + ":" + svc + " " + dbpath + " config: " + JSON.stringify( config ) );
               checkSucc = true;
            }
            catch( e )
            {
               //-145 :SDBCM_NODE_EXISTED  -290:SDB_DIR_NOT_EMPTY
               if( e == -145 || e == -290 )
               {
                  svc = svc + 5;
                  dbpath = RSRVNODEDIR + "data/" + svc;
                  failedCount++;
               }
               else
               {
                  throw "create node failed!  svc = " + svc + " dbpath = " + dbpath + " errorCode: " + e;
               }
               times++;
            }
         } while( !checkSucc && times < maxRetryTimes );
      }

      var nodeService = svc + 5;
      var nodePath = RSRVNODEDIR + "data/" + nodeService;
      for( var i = 0; i < errInstanceidList.length; i++ )
      {
         var config = { instanceid: errInstanceidList[i], diaglevel: 5 };
         try
         {
            println( "create node " + nodeHostName + ":" + nodeService + " " + nodePath + "config: " + JSON.stringify( config ) );
            rg.createNode( nodeHostName, nodeService, nodePath, config );
            throw "sessionAccess14069Error";
         }
         catch( e )
         {
            if( -6 != e )
            {
               throw buildException( "check set instanceid", e, "check the createNode and set instanceid",
                  -6, e );
            }
         }
         nodeService = nodeService + 5;
         nodePath = RSRVNODEDIR + "data/" + nodeService;
      }

      println( "begin to start rg and check result" );
      rg.start();

      // check result
      checkResult( db, groupName, instanceidList );
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      //remove rg
      println( "begin to remove rg" );
      db.removeRG( groupName );

      if( db != null )
      {
         db.close()
      }
   }
}

function checkResult ( db, groupName, instanceidList )
{
   var getDataGroupInfo = db.getRG( groupName ).getDetail().current().toObj();
   for( var i = 0; i < instanceidList.length; i++ )
   {
      if( instanceidList[i] != 0 && instanceidList[i] != getDataGroupInfo.Group[i].instanceid )
      {
         throw buildException( "check checkResult", null, "check the instanceid set result",
            instanceidList[i], getDataGroupInfo.Group[i].instanceid );
      }
   }
}

