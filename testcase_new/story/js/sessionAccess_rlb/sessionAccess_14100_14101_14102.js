/* *****************************************************************************
@discretion: 14100:setSessionAttr(),set [M/S/A] for two groups
             14101:setSessionAttr(),set instanceid for one group,the same as the other group node subscript
             14102:setSessionAttr(),set instanceid and M,the instanceid for one group             
@author��2018-1-29 wuyan  Init
***************************************************************************** */
import( "../sessionAccess/commlib.js" );
var groupName1 = "group14100a";
var groupName2 = "group14100b";
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
      var instanceidList = [3, 1, 20];
      var nodeList1 = [];
      var nodeList2 = [];
      var clName = CHANGEDPREFIX + "_sessionAcess14100";
      nodeList1 = createRGAndNode( db, groupName1, instanceidList );
      nodeList2 = createRGAndNode( db, groupName2 );
      var expSvcNameList1 = getSvcNameList( db, groupName1 );
      var expSvcNameList2 = getSvcNameList( db, groupName2 );

      //create cl ,then insert data  
      var dbcl = commCreateCLByOption( db, COMMCSNAME, clName, { ShardingKey: { no: 1 }, ReplSize: 0, Group: groupName1 } );
      //insertData( dbcl);
      splitCL( dbcl, groupName1, groupName2 );

      //set [M/S/A] for two groups
      testSessionAccess14100( db, dbcl, groupName1, groupName2 );

      //set instanceid for one group,the query for two group
      testSessionAccess14101( db, dbcl, expSvcNameList1, expSvcNameList2 );

      //set instanceid is not exist,the query for two group
      testSessionAccess14102( db, dbcl, expSvcNameList1, expSvcNameList2 );


   }
   catch( e )
   {
      println( "catch e : " + e );
      //���½�����־���ݵ�/tmp/ci/rsrvnodelogĿ¼��
      var backupDir = "/tmp/ci/rsrvnodelog/14100_" + groupName1;
      File.mkdir( backupDir );
      for( var i = 0; i < nodeList1.length; i++ )
      {
         File.scp( nodeList1[i].logSourcePath, backupDir + "/sdbdiag" + i + ".log" );
      }

      //���½�����־���ݵ�/tmp/ci/rsrvnodelogĿ¼��
      var backupDir2 = "/tmp/ci/rsrvnodelog/14100_" + groupName2;
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

function testSessionAccess14100 ( db, dbcl, groupName1, groupName2 )
{
   try
   {
      println( "---begin to test testcase14100 " );
      //test set instanceid = "M/S/A"           
      db.setSessionAttr( { PreferedInstance: ["M", "S", "A"] } );
      var queryNode = getAllAccessNode( dbcl );
      checkAccessNodeIsPrimary( queryNode[groupName1], groupName1, true );
      checkAccessNodeIsPrimary( queryNode[groupName2], groupName2, true );

      //test set instanceid = "S"               
      db.setSessionAttr( { PreferedInstance: "S" } );
      var queryNode = getAllAccessNode( dbcl );
      checkAccessNodeIsPrimary( queryNode[groupName1], groupName1, false );
      checkAccessNodeIsPrimary( queryNode[groupName2], groupName2, false );
      println( "---end to test testcase14100 " );
   }
   catch( e )
   {
      throw buildException( "testSessionAccess14100 fail", e );
   }

}

function testSessionAccess14101 ( db, dbcl, expSvcNameList1, expSvcNameList2 )
{
   try
   {
      println( "---begin to test testcase14101 " );
      var queryInstanceid = 3;
      db.setSessionAttr( { PreferedInstance: queryInstanceid } );
      var queryNode = getAllAccessNode( dbcl );
      var queryNode1 = queryNode[groupName1];
      checkAcessNodeResult( queryNode1, expSvcNameList1[0] );

      //check the second queryNode for group14100b,selection node for subscript 3
      var queryNode2 = queryNode[groupName2];
      checkAcessNodeResult( queryNode2, expSvcNameList2[2] );
      println( "---end to test testcase14101 " );
   }
   catch( e )
   {
      throw buildException( "testSessionAccess14101 fail", e );
   }
}

function testSessionAccess14102 ( db, dbcl, expSvcNameList1, expSvcNameList2 )
{
   try
   {
      println( "---begin to test testcase14102 " );
      var queryInstanceid = [20, "M"];
      db.setSessionAttr( { PreferedInstance: queryInstanceid } );
      var queryNode = getAllAccessNode( dbcl );
      var queryNode1 = queryNode[groupName1];
      checkAcessNodeResult( queryNode1, expSvcNameList1[2] );

      var queryNode2 = queryNode[groupName2];
      checkAccessNodeIsPrimary( queryNode2, groupName2, true );
      println( "---end to test testcase14102 " );
   }
   catch( e )
   {
      throw buildException( "testSessionAccess14102 fail", e );
   }

}
