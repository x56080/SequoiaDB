/* *****************************************************************************
@discretion: 14087:setSessionAttr(),set multiple instatceids,the preferedInstanceMode is ordered
             14088:setSessionAttr(),set a instatceid of array type
             14089:setSessionAttr(),set multiple instatceid ,partial instanceid does not exist
             14090:setSessionAttr(),set multiple instatceid ,the preferedInstanceMode is ordered,
                           and the first instanceid does not exist 
@author��2018-1-24 wuyan  Init
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
      var groupName = "group14087";
      var nodeList = [];
      var instanceidList = [255, 1, 20, 22];
      var nodeNum = 4;
      nodeList = createRGAndNode( db, groupName, instanceidList, nodeNum );

      //create cl ,then insert data 
      var clName = CHANGEDPREFIX + "_sessionAcess14087";
      var dbcl = commCreateCL( db, COMMCSNAME, clName, { ReplSize: 0, Group: groupName } );
      insertData( dbcl );

      //set multiple instanceid ,set the preferedInstanceMode is ordered
      println( "---begin to test testcase14087 " );
      var queryInstanceidList = [20, 22, 255];
      db.setSessionAttr( { PreferedInstance: queryInstanceidList, PreferedInstanceMode: "ordered" } );
      var expSvcNameList = getSvcNameList( db, groupName );
      checkQueryResult( db, dbcl, expSvcNameList, queryInstanceidList );
      println( "---end to test testcase14087 " );

      //test the testcace 14808
      println( "---begin to test testcase14088 " );
      var queryInstanceidList1 = [255];
      db.setSessionAttr( { PreferedInstance: queryInstanceidList1 } );
      checkAcessNodeResult( getAccessNode( dbcl ), expSvcNameList[0] );
      println( "---end to test testcase14088 " );

      //test the testcace 14809
      println( "---begin to test testcase14089 " );
      var queryInstanceidList2 = [29, 255];
      db.setSessionAttr( { PreferedInstance: queryInstanceidList2 } );
      //query node instanceid is 255
      checkAcessNodeResult( getAccessNode( dbcl ), expSvcNameList[0] );
      //get session
      var expSessionInfo = { PreferedInstance: queryInstanceidList2, PreferedInstanceMode: "ordered", "Timeout": -1 };
      getSessionAndCheckResult( db, expSessionInfo );
      println( "---end to test testcase14089 " );

      //test the testcace 14810
      println( "---begin to test testcase14090 " );
      var queryInstanceidList3 = [101, 22, 1];
      db.setSessionAttr( { PreferedInstance: queryInstanceidList3, PreferedInstanceMode: "ordered" } );
      checkAcessNodeResult( getAccessNode( dbcl ), expSvcNameList[3] );
      println( "---end to test testcase14090 " );
   }
   catch( e )
   {
      println( "catch e : " + e );
      //���½�����־���ݵ�/tmp/ci/rsrvnodelogĿ¼��
      var backupDir = "/tmp/ci/rsrvnodelog/14087";
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

function checkQueryResult ( db, dbcl, expSvcNameList, queryInstanceidList )
{
   //check the result   
   //instanceid is 20,the node is expSvcNameList[2]          
   var expAccessNode = expSvcNameList[2];
   var actAccessNode = getAccessNode( dbcl );
   checkAcessNodeResult( actAccessNode, expAccessNode );
   //get session
   var expSessionInfo = { PreferedInstance: queryInstanceidList, PreferedInstanceMode: "ordered", "Timeout": -1 };
   getSessionAndCheckResult( db, expSessionInfo );

}