/* *****************************************************************************
@discretion: setSessionAttr(),set instatceid is node subscript
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
      var groupName = "group14084";
      var nodeList = [];
      nodeList = createRGAndNode( db, groupName );
      var expSvcNameList = getSvcNameList( db, groupName );

      //create cl and insert data
      var clName = CHANGEDPREFIX + "_sessionAcess14084";
      var dbcl = commCreateCLByOption( db, COMMCSNAME, clName, { ReplSize: 0, Group: groupName }, true, true );
      insertData( dbcl );

      //set instanceid 
      println( "---begin to set instanceid " );
      var instanceid = Math.floor( Math.random() * expSvcNameList.length + 1 );
      db.setSessionAttr( { PreferedInstance: instanceid } )

      //check the query node     
      var queryNode = getAccessNode( dbcl );
      var expNodeName = expSvcNameList[instanceid - 1];
      checkAcessNodeResult( queryNode, expNodeName );
      println( "---end to set instanceid " );
   }
   catch( e )
   {
      println( "catch e : " + e );
      //���½�����־���ݵ�/tmp/ci/rsrvnodelogĿ¼��
      var backupDir = "/tmp/ci/rsrvnodelog/14084";
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

