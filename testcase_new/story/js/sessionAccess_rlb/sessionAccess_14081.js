/* *****************************************************************************
@discretion: setSessionAttr(),set a instatceid
@author��2018-1-22 wuyan  Init
***************************************************************************** */
import( "../sessionAccess/commlib.js" );
main();
function main ()
{
   try
   {
      var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
      if( true == commIsStandalone( db ) )
      {
         println( "run mode is standalone" );
         return;
      }

      //create group and node  
      var groupName = "group14081";
      var nodeList = [];
      var instanceidList = [0, 0, 15];
      nodeList = createRGAndNode( db, groupName, instanceidList );

      //create cl and insert data
      var clName = CHANGEDPREFIX + "_sessionAcess14081";
      var options = { ReplSize: 0, Group: groupName };
      var dbcl = commCreateCLByOption( db, COMMCSNAME, clName, options, true, true );
      insertData( dbcl );

      //set session instanceid 
      var instanceid = 15;
      db.setSessionAttr( { PreferedInstance: instanceid } )

      //check the query node 
      var queryNode = getAccessNode( dbcl );
      println( "queryNode=" + queryNode );
      var expSvcNameList = getSvcNameList( db, groupName );
      var expNodeName = expSvcNameList[2];
      checkAcessNodeResult( queryNode, expNodeName );

      //get session and check result
      var expSessionInfo = { PreferedInstance: 15, PreferedInstanceMode: "random", "Timeout": -1 };
      getSessionAndCheckResult( db, expSessionInfo );
   }
   catch( e )
   {
      println( "catch e : " + e );
      //���½�����־���ݵ�/tmp/ci/rsrvnodelogĿ¼��
      var backupDir = "/tmp/ci/rsrvnodelog/14081";
      File.mkdir( backupDir );
      for( var i = 0; i < nodeList.length; i++ )
      {
         File.scp( nodeList[i].logSourcePath, backupDir + "/sdbdiag" + i + ".log" );
      }
      throw e;
   }
   finally
   {
      commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the end" );
      db.removeRG( groupName );

      if( db != null )
      {
         db.close()
      }
   }
}

