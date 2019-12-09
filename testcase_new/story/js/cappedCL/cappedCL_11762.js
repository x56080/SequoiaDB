/************************************
*@Description: create cappedCS and drop cappedCS
*@author:      luweikang
*@createdate:  2017.7.4
*@testlinkCase:seqDB-11762
**************************************/

main();

function main ()
{
   var csName = COMMCAPPEDCSNAME + "_11762";

   //clean environment before test
   commDropCS( db, csName, true, "drop CS in the beginning" );

   //begin create cappedCS
   println( "---begin to create cappedCS---" )
   var options = { Capped: true }
   commCreateCS( db, csName, false, "beginning to create cappedCS", options );

   //check create result
   if( true === commIsStandalone( db ) )
   {
      standaloneCheckCreateCS( csName );
   }
   else
   {
      var nodeList = getNodeList( CATALOG_GROUPNAME );
      checkCappedCS( csName, nodeList );
   }
   println( "---end create cappedCS---" )

   //begin to drop cappedCS 
   println( "---begin to drop cappedCS---" )
   commDropCS( db, csName, false, "beginning to drop cappedCS" );

   //check drop result
   if( true === commIsStandalone( db ) )
   {
      standaloneCheckDropCS( csName );
   }
   else
   {
      checkDropCS( csName, nodeList );
   }
   println( "---end to drop cappedCS---" );
}

function standaloneCheckCreateCS ( csName )
{
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { 'Name': csName } );
   var type = cursor.next().toObj().Type;
   if( type !== 1 )
   {
      throw buildException( "check cappedCS", null, "check cappedCS", "1", type );
   }
   cursor.close();
}

function standaloneCheckDropCS ( csName )
{
   try
   {
      db.getCS( csName );
      throw "ERR_DROP_CS";
   }
   catch( e )
   {
      if( e !== -34 )
      {
         throw buildException( "check drop cappedCS", null, "check drop cappedCS", "-34", e );
      }
   }
}

function getNodeList ( groupName )
{
   var nodeList = [];
   var groupInfo = db.getRG( groupName ).getDetail().current().toObj().Group;
   for( var i in groupInfo )
   {
      var nodeInfo = groupInfo[i].HostName + ":" + groupInfo[i].Service[0].Name;
      nodeList.push( nodeInfo );
   }
   //println( "---get nodelist of " + groupName + ": " + nodeList );

   return nodeList;
}

function checkCappedCS ( csName, nodeList )
{
   var repeatTime = 10;
   var clSize = 0;
   for( var i = 0; i < repeatTime; i++ )
   {
      var j = i % nodeList.length;
      println( "j: " + j );
      var catadb = new Sdb( nodeList[j] );
      clSize = catadb.SYSCAT.SYSCOLLECTIONSPACES.count( { 'Name': csName } );
      //judge the current node exist CL or not 
      if( clSize == 0 )
      {
         // wait for the slave node sync
         sleep( 2 * 60 * 1000 );//2 mins		
         clSize = catadb.SYSCAT.SYSCOLLECTIONSPACES.count( { 'Name': csName } );
      }

      if( clSize != 0 )
      {
         var cursor = catadb.SYSCAT.SYSCOLLECTIONSPACES.find( { 'Name': csName } );
         var type = cursor.next().toObj().Type;
         if( type !== 1 )
         {
            throw buildException( "check cappedCS", null, "check cappedCS", "1", type );
         }
         cursor.close();
      } else
      {
         throw buildException( "check cappedCL failed , cursor is null" );
      }
      catadb.close();
   }
}

function checkDropCS ( csName, nodeList )
{
   for( var i in nodeList )
   {
      var catadb = new Sdb( nodeList[i] );
      var count = catadb.SYSCAT.SYSCOLLECTIONSPACES.count( { 'Name': csName } )
      if( count != 0 )
      {
         throw buildException( "check drop cappedCS", null, "check drop cappedCS", "0", count );
      }
      catadb.close();
   }
}
