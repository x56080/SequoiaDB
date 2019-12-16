/* *****************************************************************************
@discretion: setSessionAttr(),set preferedStrict = true, assign node select when the node stop.
@author：2019-6-20 luweikang  Init
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
      //create group and node
      var csName = CHANGEDPREFIX + "_cs18601";
      var groupName = "group18601";
      var instanceidList = [9, 8, 10, 11];
      var nodeNum = 4;
      var nodeList = createRGAndNode( db, groupName, instanceidList, nodeNum );
      var expSvcNameList = getSvcNameList( db, groupName );

      //create cl ,then insert data 
      var clName = CHANGEDPREFIX + "_cl18601";
      var dbcl = commCreateCL( db, csName, clName, { ReplSize: 0, Group: groupName } );
      insertData( dbcl );

      println( "---begin to set and query instanceid is " + instanceidList[1] + " and " + instanceidList[2] );
      db.setSessionAttr( { PreferedInstance: [instanceidList[1], instanceidList[2]], PreferedStrict: true } );

      //stop node and select record
      println( "begin to stop node: " + expSvcNameList[1] );
      var rg = db.getRG( groupName );
      rg.getNode( nodeList[1].hostname, nodeList[1].svcname ).stop();
      setSessionAttrAndCheckResult( dbcl, expSvcNameList[2] );

      try
      {
         println( "begin to stop node: " + expSvcNameList[2] );
         rg.getNode( nodeList[2].hostname, nodeList[2].svcname ).stop();
         dbcl.find().explain();
         throw "FIND_SHOULD_FAIL";
      }
      catch( e )
      {
         if( e != -250 )
         {
            throw e;
         }
      }
      db.setSessionAttr( { PreferedStrict: false } );
      dbcl.find().explain();

      //restart the node and select record
      println( "restart the group: " + groupName );
      rg.start();
      db.setSessionAttr( { PreferedStrict: true } );
      dbcl.find().explain();
   }
   catch( e )
   {
      println( "catch e : " + e );
      //将新建组日志备份到/tmp/ci/rsrvnodelog目录下
      var backupDir = "/tmp/ci/rsrvnodelog/18601";
      File.mkdir( backupDir );
      for( var i = 0; i < nodeList.length; i++ )
      {
         File.scp( nodeList[i].logSourcePath, backupDir + "/sdbdiag" + i + ".log" );
      }
      throw e;
   }
   finally
   {
      rg.start();
      commDropCS( db, csName, true );
      db.removeRG( groupName );
   }
}

function setSessionAttrAndCheckResult ( dbcl, expQueryNode )
{
   var queryNode = getAccessNode( dbcl );
   checkAcessNodeResult( queryNode, expQueryNode );
}