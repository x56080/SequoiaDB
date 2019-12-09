/******************************************************************************
@Description : Test stop minority nodes and the minority nodes have
               primary node in the group.
@Modify list :
               2014-6-12  xiaojun Hu  Init
******************************************************************************/
function testOneGroup ( group )
{
   if( group.length <= 3 ) return;
   var nodeSize = group.length;
   var getRG = group[0].GroupName;         // GroupName
   var primNode = group[0].PrimaryNode;    // PrimaryNode
   var result = true;
   var primaryPos = 0;

   println( "***Start to inspect the voting in group = [" + getRG + " ]***" );
   var mino = 0;
   // 1.Stop minority nodes
   for( var j = 1; j < nodeSize; ++j )    //many groups,begin 1 not 0
   {
      var nodeID = group[j].NodeID;    // NodeID
      var node = group[j].svcname;    // svcname
      var nodeHost = group[j].HostName;    // HostName
      if( primNode == nodeID )
      {
         var primHost = group[j].HostName;
         var master = group[j].svcname;    // Master node
         primaryPos = j;
         continue;
      }

      if( mino++ < Math.floor( ( nodeSize - 1 ) / 2 ) )
      {
         // Stop no primary node
         stopNode( db, getRG, nodeHost, node );
      }
   }

   // 2.Stop primary node
   stopNode( db, getRG, primHost, master );

   // 3.Inspect the new primary node ant the olde primary node
   try
   {
      havePrimInGroup( db, getRG );
   }
   catch( e )
   {
      if( e === "Cannot createCL success" ) throw e;
   }
   println( "Inspect Over" );

   // add 1 because beginpos=1
   var endPos = Math.floor( ( nodeSize - 1 ) / 2 ) + 1;
   if( primaryPos <= endPos )
   {
      ++endPos
   }
   // 4.Start minority nodes
   for( var j = 1; j < endPos; ++j )    //many groups,begin 1 not 0
   {
      if( j == primaryPos ) continue;
      var node = group[j].svcname;    // svcname
      var nodeHost = group[j].HostName;    // HostName
      startNode( db, getRG, nodeHost, node );
   }

   // 4.Start primary node
   startNode( db, getRG, primHost, master );

   // Inspect the new primary node ant the olde primary node
   var majCount = 0;
   var sleepTimeLen = 60;
   do
   {
      sleep( 1000 );
      try
      {
         havePrimInGroup( db, getRG );
      }
      catch( e )
      {
         if( "Cannot createCL success" != e )
            throw e;
         else break;
      }
      ++majCount;
   } while( sleepTimeLen > majCount );

   if( sleepTimeLen == majCount )
      throw "Don't have primary node in the end";

   println( "Start majority nodes, then have Primary node" );
   clearGroup( db, getRG );

}

function main ( db )
{
   // Get Primary node related infomation
   var groups = commGetGroups( db, "", "", true, true );
   var rgSize = groups.length;
   println( " Group Size : " + rgSize );
   for( var i = 0; i < rgSize; ++i )
   {
      testOneGroup( groups[i] );
   }
}

// Main Running
try
{
   var mode = commIsStandalone( db );
   if( false == mode )
      main( db );
   else
      println( "Run Mode is : Standalone" );
}
catch( e )
{
   throw e;
}
