/* *****************************************************************************
@discretion: setAttr() is M/S/A,the session is specified in the configuration(A is default and testover)
@author��2018-1-24 wuyan  Init
***************************************************************************** */
import( "../sessionAccess/commlib.js" );
main();
function main ()
{
   var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   if( true == commIsStandalone( db ) )
   {
      println( "run mode is standalone" );
      return;
   }

   try
   {
      var groupName = "group14082";
      var nodeList = [];
      var coordLogSourcePaths = [];
      var coordRg = db.getRG( "SYSCoord" );
      var nodeHostName = db.listReplicaGroups().current().toObj().Group[0].HostName;
      var nodeService = parseInt( RSRVPORTBEGIN ) + 200;
      var nodeService1 = parseInt( RSRVPORTBEGIN ) + 210;
      var path = RSRVNODEDIR + "coord/" + nodeService;

      //create data group and nodes
      nodeList = createRGAndNode( db, groupName );

      //create coord node
      coordRg.createNode( nodeHostName, nodeService, path, { preferedinstance: "M" } );
      coordLogSourcePaths.push( nodeHostName + ":" + CMSVCNAME + "@" + path + "/diaglog/sdbdiag.log" );
      path = RSRVNODEDIR + "coord/" + nodeService1;
      coordRg.createNode( nodeHostName, nodeService1, path, { preferedinstance: "S" } );
      coordLogSourcePaths.push( nodeHostName + ":" + CMSVCNAME + "@" + path + "/diaglog/sdbdiag.log" );
      println( "coordRg start..." );
      coordRg.start();

      //create cl ,then insert data 
      var clName = CHANGEDPREFIX + "_sessionAcess14082";
      var dbcl = commCreateCL( db, COMMCSNAME, clName, { ReplSize: 0, Group: groupName } );
      insertData( dbcl );
      //test sessionAttr is M on coord
      var coordUrl = new Sdb( nodeHostName, nodeService );
      var queryInstanceid = "M";
      testsessionAccess( coordUrl, clName, groupName, queryInstanceid, true );

      //test sessionAttr is S on coord
      var coordUrl1 = new Sdb( nodeHostName, nodeService1 );
      var queryInstanceid1 = "S";
      testsessionAccess( coordUrl1, clName, groupName, queryInstanceid1, false );
   }
   catch( e )
   {
      println( "catch e : " + e );
      //���½�����־���ݵ�/tmp/ci/rsrvnodelogĿ¼��
      var backupDir = "/tmp/ci/rsrvnodelog/14082";
      File.mkdir( backupDir );
      for( var i = 0; i < nodeList.length; i++ )
      {
         File.scp( nodeList[i].logSourcePath, backupDir + "/sdbdiag" + i + ".log" );
      }

      //���½�coord�ڵ���־���ݵ�/tmp/ci/rsrvnodelogĿ¼��
      var backupCoordDir = "/tmp/ci/rsrvnodelog/14082_coord";
      File.mkdir( backupCoordDir );
      for( var i = 0; i < coordLogSourcePaths.length; i++ )
      {
         File.scp( coordLogSourcePaths[i], backupCoordDir + "/sdbdiag" + i + ".log" );
      }
      throw e;
   }
   finally
   {
      commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the end" );
      db.removeRG( groupName );
      try
      {
         coordRg.removeNode( nodeHostName, nodeService );
      }
      catch( e )
      {
         if( e !== -155 )
         {
            throw "coordRg remove node 1 catch e : " + e;
         }
      }

      try
      {
         coordRg.removeNode( nodeHostName, nodeService1 );
      }
      catch( e )
      {
         if( e !== -155 )
         {
            throw "coordRg remove node 2 catch e : " + e;
         }
      }
      if( coordUrl != null )
      {
         coordUrl.close()
      }
      if( coordUrl1 != null )
      {
         coordUrl1.close()
      }
   }
}

function testsessionAccess ( coordUrl, clName, groupName, queryInstanceid, isPrimary )
{
   try
   {
      println( "---begin to test query " + queryInstanceid );
      //test setsession       
      var querydb = coordUrl.getCS( COMMCSNAME ).getCL( clName );
      var actAccessNode = getAccessNode( querydb );
      checkAccessNodeIsPrimary( actAccessNode, groupName, isPrimary );

      //test getsession
      var expSessionInfo = { PreferedInstance: queryInstanceid, PreferedInstanceMode: "random", "Timeout": -1 };
      getSessionAndCheckResult( coordUrl, expSessionInfo );
      println( "---end to test query " + queryInstanceid );
   }
   catch( e )
   {
      throw buildException( "testSessionAccess14082 fail", e );
   }
}

