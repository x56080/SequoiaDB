/* *****************************************************************************
@discretion: setSessionAttr(),set instatceid is same as the other node subscript
@author��2018-1-24 wuyan  Init
***************************************************************************** */
import( "../sessionAccess/commlib.js" );
main();

function main ()
{
   try
   {
      var clName = CHANGEDPREFIX + "_sessionAcess14085";
      var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );

      //create group and node
      var groupName = "group14085";
      var nodeList = [];
      var instanceidList = [2, 0, 0];
      nodeList = createRGAndNode( db, groupName, instanceidList );

      //create cl ,then insert data  
      var dbcl = commCreateCL( db, COMMCSNAME, clName, { ReplSize: 0, Group: groupName } );
      insertData( dbcl );

      //set one node instanceid is 2,the same as the nodesubscript is 2
      println( "---begin to set instanceid " );
      var instanceid = 2;
      var accessCount = {};
      var expSvcNameList = getSvcNameList( db, groupName );
      var expAccessNode = [expSvcNameList[0], expSvcNameList[instanceid - 1]];
      for( var i = 0; i < 20; i++ ) 
      {
         db.setSessionAttr( { PreferedInstance: instanceid } );
         var actAccessNode = getAccessNode( dbcl );
         checkAcessNodeResult( actAccessNode, expAccessNode );
         storageNodeAccessCount( actAccessNode, accessCount );
      }
      checkRandomAccessResult( expAccessNode, accessCount );
      println( "---end to set instanceid " );
   }
   catch( e )
   {
      println( "catch e : " + e );
      //���½�����־���ݵ�/tmp/ci/rsrvnodelogĿ¼��
      var backupDir = "/tmp/ci/rsrvnodelog/14085";
      File.mkdir( backupDir );
      for( var i = 0; i < nodeList.length; i++ )
      {
         File.scp( nodeList[i].logSourcePath, backupDir + "/sdbdiag" + i + ".log" );
      }
      throw e;
   }
   finally
   {
      if( db != null )
      {
         commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the end" );
         db.removeRG( groupName );
         db.close()
      }
   }
}

