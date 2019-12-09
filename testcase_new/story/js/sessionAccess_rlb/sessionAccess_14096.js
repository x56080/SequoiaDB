/* *****************************************************************************
@discretion: setSessionAttr(),set instanceid is 8/9/10( old version 8 is master,9 is slave,
               new version query node by instanceid)
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
      var groupName = "group14096";
      var nodeList = [];
      var instanceidList = [9, 8, 10, 11];
      var nodeNum = 4;
      var clName = CHANGEDPREFIX + "_sessionAcess14096";
      nodeList = createRGAndNode( db, groupName, instanceidList, nodeNum );
      var expSvcNameList = getSvcNameList( db, groupName );

      //create cl ,then insert data  
      var dbcl = commCreateCLByOption( db, COMMCSNAME, clName, { ReplSize: 0, Group: groupName } );
      insertData( dbcl );

      //set instanceid is 8,the query node is slave node
      var queryInstanceid = 8;
      setSessionAttrAndCheckResult( db, dbcl, groupName, queryInstanceid, expSvcNameList[1], false );

      //set instanceid is 9,the query node is master node
      var queryInstanceid1 = 9;
      setSessionAttrAndCheckResult( db, dbcl, groupName, queryInstanceid1, expSvcNameList[0], true );

      //set instanceid is 10,the query node is slave node
      var queryInstanceid2 = 10;
      setSessionAttrAndCheckResult( db, dbcl, groupName, queryInstanceid2, expSvcNameList[2], false );
   }
   catch( e )
   {
      println( "catch e : " + e );
      //���½�����־���ݵ�/tmp/ci/rsrvnodelogĿ¼��
      var backupDir = "/tmp/ci/rsrvnodelog/14096";
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

function setSessionAttrAndCheckResult ( db, dbcl, groupName, queryInstanceid, expQueryNode, isPrimary )
{
   println( "---begin to set and query instanceid is " + queryInstanceid );
   db.setSessionAttr( { PreferedInstance: queryInstanceid } );
   var queryNode = getAccessNode( dbcl );
   checkAcessNodeResult( queryNode, expQueryNode );
   checkAccessNodeIsPrimary( queryNode, groupName, isPrimary );
   println( "---end to set and query instanceid is " + queryInstanceid );
}