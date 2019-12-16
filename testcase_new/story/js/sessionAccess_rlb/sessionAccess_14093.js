/* *****************************************************************************
@discretion: setSessionAttr(),set instanceid and [M/S/A],the preferedInstanceMode is ordered
             test the following scenes:
             a: set multiple instanceid and ["M"]/["m"]
             b: set multiple instanceid and ["S"]/["s"]
             c: set multiple instanceid and ["A"]/["a"]
             d: set multiple instanceid and [M/S/A]
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
      var groupName = "group14093";
      var nodeList = [];
      var instanceidList = [30, 124, 8, 22];
      var nodeNum = 4;
      var clName = CHANGEDPREFIX + "_sessionAcess14093";
      nodeList = createRGAndNode( db, groupName, instanceidList, nodeNum );
      //expSvcNameList[i] : instanceidList[i]
      var expSvcNameList = getSvcNameList( db, groupName );

      //create cl ,then insert data  
      var dbcl = commCreateCL( db, COMMCSNAME, clName, { ReplSize: 0, Group: groupName } );
      insertData( dbcl );

      //a: set multiple instanceid and ["M"]
      println( "---begin to test set multiple instanceid and ['M']/['m'] " );
      var queryInstanceidList = [124, 8, 30, "M"];
      var expQueryNode_a1 = expSvcNameList[0];
      setSessionAttrAndCheckResult( db, dbcl, queryInstanceidList, expQueryNode_a1 )
      //a: set multiple instanceid and ["m"] 
      var queryInstanceidList_a2 = [124, 8, 30, "m"];
      var expQueryNode_a2 = expSvcNameList[1];
      setSessionAttrAndCheckResult( db, dbcl, queryInstanceidList_a2, expQueryNode_a2 );
      println( "---end to test set multiple instanceid and ['M']/['m'] " );

      //b: set multiple instanceid and ["S"]
      println( "---begin to test set multiple instanceid and ['S']/['s'] " );
      var queryInstanceidList_b1 = [124, 8, 30, "S"];
      var expQueryNode_b1 = expSvcNameList[1];
      setSessionAttrAndCheckResult( db, dbcl, queryInstanceidList_b1, expQueryNode_b1 );
      //b: set multiple instanceid and ["s"],the instanceid:30 is masterNode
      var queryInstanceidList_b2 = [30, 124, 8, "s"];
      var expQueryNode_b2 = expSvcNameList[0];
      setSessionAttrAndCheckResult( db, dbcl, queryInstanceidList_b2, expQueryNode_b2 );
      println( "---end to test set multiple instanceid and ['S']/['s'] " );

      //c: set multiple instanceid and ["A"]
      println( "---begin to test set multiple instanceid and ['A']/['a'] " );
      var queryInstanceidList_c1 = [124, 8, 30, "A"];
      var expQueryNode_c1 = expSvcNameList[1];
      setSessionAttrAndCheckResult( db, dbcl, queryInstanceidList_c1, expQueryNode_c1 );
      //c: set multiple instanceid and ["a"],the instanceid:30 is masternode
      var queryInstanceidList_c2 = [22, 124, 8, "a"];
      var expQueryNode_c2 = expSvcNameList[3];
      setSessionAttrAndCheckResult( db, dbcl, queryInstanceidList_c2, expQueryNode_c2 );
      println( "---end to test set multiple instanceid and ['A']/['a'] " );

      //d: set multiple instanceid and ["M/S/A"]
      println( "---begin to test set multiple instanceid and ['M/S/A'] " );
      var queryInstanceidList_d = [124, 8, 30, "M", "S", "A"];
      var expQueryNode_d = expSvcNameList[0];
      setSessionAttrAndCheckResult( db, dbcl, queryInstanceidList_d, expQueryNode_d );
      println( "---end to test set multiple instanceid and ['M/S/A'] " );
   }
   catch( e )
   {
      println( "catch e : " + e );
      //���½�����־���ݵ�/tmp/ci/rsrvnodelogĿ¼��
      var backupDir = "/tmp/ci/rsrvnodelog/14093";
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

function setSessionAttrAndCheckResult ( db, dbcl, queryInstanceidList, expQueryNode )
{
   db.setSessionAttr( { PreferedInstance: queryInstanceidList, PreferedInstanceMode: "ordered" } );
   checkAcessNodeResult( getAccessNode( dbcl ), expQueryNode );
}