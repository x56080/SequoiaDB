/* *****************************************************************************
@discretion: 14097:setSessionAttr(),set instanceid for two groups
             14098:setSessionAttr(),set instanceid for one group,query for two groups
             14099:setSessionAttr(),set instanceid is not exist,query for two groups
@author��2018-1-29 wuyan  Init
***************************************************************************** */
import( "../sessionAccess/commlib.js" );
var groupName1 = "group14097a";
var groupName2 = "group14097b";
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
      var instanceidList1 = [255, 1, 20];
      var instanceidList2 = [253, 1, 23];
      var nodeList1 = [];
      var nodeList2 = [];
      var nodeNum = 3;
      var clName = CHANGEDPREFIX + "_sessionAcess14097";
      nodeList1 = createRGAndNode( db, groupName1, instanceidList1, nodeNum );
      nodeList2 = createRGAndNode( db, groupName2, instanceidList2, nodeNum );
      var expSvcNameList1 = getSvcNameList( db, groupName1 );
      var expSvcNameList2 = getSvcNameList( db, groupName2 );

      //create cl ,then insert data
      var dbcl = commCreateCL( db, COMMCSNAME, clName, { ShardingKey: { no: 1 }, ReplSize: 0, Group: groupName1 } );
      insertData( dbcl );
      splitCL( dbcl, groupName1, groupName2 );

      //set instanceid for two groups
      testSessionAccess14097( db, dbcl, expSvcNameList1, expSvcNameList2 );

      //set instanceid for one group,the query for two group
      testSessionAccess14098( db, dbcl, expSvcNameList1, expSvcNameList2 );

      //set instanceid is not exist,the query for two group
      testSessionAccess14099( db, dbcl, expSvcNameList1, expSvcNameList2 );
   }
   catch( e )
   {
      println( "catch e : " + e );
      //���½�����־���ݵ�/tmp/ci/rsrvnodelogĿ¼��
      var backupDir = "/tmp/ci/rsrvnodelog/14097_" + groupName1;
      File.mkdir( backupDir );
      for( var i = 0; i < nodeList1.length; i++ )
      {
         File.scp( nodeList1[i].logSourcePath, backupDir + "/sdbdiag" + i + ".log" );
      }

      //���½�����־���ݵ�/tmp/ci/rsrvnodelogĿ¼��
      var backupDir2 = "/tmp/ci/rsrvnodelog/14097_" + groupName2;
      File.mkdir( backupDir2 );
      for( var i = 0; i < nodeList2.length; i++ )
      {
         File.scp( nodeList2[i].logSourcePath, backupDir2 + "/sdbdiag" + i + ".log" );
      }
      throw e;
   }
   finally
   {
      commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the end" );
      db.removeRG( groupName1 );
      db.removeRG( groupName2 );
      if( db != null )
      {
         db.close()
      }
   }
}

function testSessionAccess14097 ( db, dbcl, expSvcNameList1, expSvcNameList2 )
{
   try
   {
      println( "---begin to test testcase14097 " );
      var queryInstanceid = 1;
      db.setSessionAttr( { PreferedInstance: queryInstanceid } );
      var expNodeName = [expSvcNameList1[1], expSvcNameList2[1]];
      var queryNode = getAllAccessNode( dbcl );
      var actNodeName = [queryNode[groupName1], queryNode[groupName2]];
      checkAllAcessNode( dbcl, actNodeName, expNodeName );
      println( "---end to test testcase14097 " );
   }
   catch( e )
   {
      throw buildException( "testSessionAccess14097 fail", e );
   }

}

function testSessionAccess14098 ( db, dbcl, expSvcNameList1, expSvcNameList2 )
{
   try
   {
      println( "---begin to test testcase14098 " );
      var queryInstanceid = [255, 13];
      var accessCount = {};
      for( var i = 0; i < 20; i++ ) 
      {
         db.setSessionAttr( { PreferedInstance: queryInstanceid } );
         //check the fisrt queryNode for group14097a
         var queryNode = getAllAccessNode( dbcl );
         var queryNode1 = queryNode[groupName1];
         checkAcessNodeResult( queryNode1, expSvcNameList1[0] );
         //check the second queryNode for group14097b,random selection node for groupb
         var queryNode2 = queryNode[groupName2];
         storageNodeAccessCount( queryNode2, accessCount );
      }
      checkRandomAccessResult( expSvcNameList2, accessCount );
      println( "---end to test testcase14098 " );
   }
   catch( e )
   {
      throw buildException( "testSessionAccess14098 fail", e );
   }
}

function testSessionAccess14099 ( db, dbcl, expSvcNameList1, expSvcNameList2 )
{
   try
   {
      println( "---begin to test testcase14099 " );
      var queryInstanceid = [155, 244];
      var accessCount1 = {};
      var accessCount2 = {};
      for( var i = 0; i < 20; i++ ) 
      {
         db.setSessionAttr( { PreferedInstance: queryInstanceid } );
         //check the fisrt queryNode for group14097a,random selection node for groupb
         var queryNode = getAllAccessNode( dbcl );
         var queryNode1 = queryNode[groupName1];
         storageNodeAccessCount( queryNode1, accessCount1 );
         //check the second queryNode for group14097b,random selection node for groupb
         var queryNode2 = queryNode[groupName2];
         storageNodeAccessCount( queryNode2, accessCount2 );
      }
      checkRandomAccessResult( expSvcNameList1, accessCount1 );
      checkRandomAccessResult( expSvcNameList2, accessCount2 );
      println( "---end to test testcase14099 " );
   }
   catch( e )
   {
      throw buildException( "testSessionAccess14099 fail", e );
   }

}
