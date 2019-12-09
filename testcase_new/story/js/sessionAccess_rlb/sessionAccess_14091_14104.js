/* *****************************************************************************
@discretion: 14091:setSessionAttr(),set [M/S/A],the preferedInstanceMode is random
                   setSessionAttr(),set [S/M/A],the preferedInstanceMode is random
             14104:set sessionAttr is S after insert data
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
      var groupName = "group14091";
      var nodeList = [];
      var csName = CHANGEDPREFIX + "_cs14091";
      nodeList = createRGAndNode( db, groupName );

      //create cs/maincl/subcl/attchcl ,then insert data 
      var mainCLName = CHANGEDPREFIX + "_maincl14091";
      var subclName1 = CHANGEDPREFIX + "_sessionAcess14091a";
      var subclName2 = CHANGEDPREFIX + "_sessionAcess14091b";
      commCreateCS( db, csName, false, "Failed to create CS." );
      var mainCL = createMainCL( db, groupName, csName, mainCLName )
      createSubCL( db, groupName, csName, subclName1 );
      createSubCL( db, groupName, csName, subclName2 );
      attachCL( csName, mainCL, subclName1, 0, 3000 );
      attachCL( csName, mainCL, subclName2, 3000, 6000 );
      insertData( mainCL );

      println( "---begin to test testcase14091" );
      //set [M/S/A] ,set the preferedInstanceMode is ordered 
      var queryInstanceidList = ["M", "S", "A"];
      db.setSessionAttr( { PreferedInstance: queryInstanceidList, PreferedInstanceMode: "random" } );
      var actAccessNode = getAccessNode( mainCL );
      checkAccessNodeIsPrimary( actAccessNode, groupName, true );

      //set [S/M/A] ,set the preferedInstanceMode is ordered(test 14104)
      println( "---begin to set the [S/M/A]" );
      var queryInstanceidList1 = ["S", "M", "A"];
      db.setSessionAttr( { PreferedInstance: queryInstanceidList1, PreferedInstanceMode: "random" } );
      var actAccessNode = getAccessNode( mainCL );
      checkAccessNodeIsPrimary( actAccessNode, groupName, false );
      println( "---end to test testcase14091 " );

      commDropCS( db, csName, false, "Failed to drop CS." );
   }
   catch( e )
   {
      println( "catch e : " + e );
      //���½�����־���ݵ�/tmp/ci/rsrvnodelogĿ¼��
      var backupDir = "/tmp/ci/rsrvnodelog/14091";
      File.mkdir( backupDir );
      for( var i = 0; i < nodeList.length; i++ )
      {
         File.scp( nodeList[i].logSourcePath, backupDir + "/sdbdiag" + i + ".log" );
      }
      throw e;
   }
   finally
   {
      commDropCS( db, csName, true, "Failed to drop CS." );
      db.removeRG( groupName );

      if( db != null )
      {
         db.close()
      }
   }
}

function createMainCL ( db, groupName, csName, mainCLName )
{
   var options = { ShardingKey: { no: 1 }, ReplSize: 0, Group: groupName, IsMainCL: true };
   var mainCL = commCreateCLByOption( db, csName, mainCLName, options, false,
      true, "Failed to create mainCL." );
   return mainCL;
}

function createSubCL ( db, groupName, csName, subCLName )
{
   var options = {
      ShardingKey: { a: 1 }, ShardingType: "hash",
      ReplSize: 0, Group: groupName, Compressed: true
   };
   var subCL = commCreateCLByOption( db, csName, subCLName, options, false,
      true, "Failed to create subCL." );
   return subCL;
}

function attachCL ( csName, mainCL, subCLName, beginNo, endNo )
{
   var options = { LowBound: { "no": beginNo }, UpBound: { "no": endNo } };
   mainCL.attachCL( csName + "." + subCLName, options );
}



