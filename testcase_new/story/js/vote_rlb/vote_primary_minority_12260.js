/******************************************************************************
@Description : Test stop minority nodes and the minority nodes have
               primary node in the group.
@Modify list :
               2014-6-12  xiaojun Hu  Init
******************************************************************************/

function main ( db )
{
   // Get Primary node related infomation
   var group = commGetGroups( db );
   var rgSize = group.length;
   println( " Group Size : " + rgSize );
   for( var i = 0; i < rgSize; ++i )
   {
      var nodeSize = group[i].length;
      var getRG = group[i][0].GroupName;         // GroupName
      var primNode = group[i][0].PrimaryNode;    // PrimaryNode
      var result = true;
      // If the nodes less than 3, nodes cannot be stop
      if( 3 < nodeSize )
      {
         var mino = 0;
         println( "node size : " + nodeSize );
         for( var j = 1; j < nodeSize; ++j )    //many groups,begin 1 not 0
         {
            ++mino;
            var nodeID = group[i][j].NodeID;    // NodeID
            var node = group[i][j].svcname;    // svcname
            var nodeHost = group[i][j].HostName;    // HostName
            if( primNode == nodeID )
            {
               var primHost = group[i][j].HostName;
               var master = group[i][j].svcname;    // Master node
               continue;
            }
            if( mino < Math.floor( nodeSize / 2 ) - 1 )
            {
               // Stop no primary node
               println( "begin, node? : " + node );
               stopNode( db, getRG, nodeHost, node );
            }
         }
         // Stop primary node
         stopNode( db, getRG, primHost, master );


         // Inspect the new primary node ant the olde primary node
         var count = 0;
         var totalSleepLen = 60;
         do
         {
            ++count;
            sleep( 1000 );
            var newPrimNode = getPrimNode( db, getRG );
            //println( "node ID" + newPrimNode + " = " + primNode ) ;
            if( totalSleepLen < count )
            {
               result = false;
            }
            //println( "count : " + count ) ;
         } while( false == newPrimNode );

         // Stop primary node
         startNode( db, getRG, primHost, master );

         for( var j = 1; j < Math.floor( nodeSize / 2 ) - 1; ++j )    //many groups,begin 1 not 0
         {
            var node = group[i][j].svcname;    // svcname
            var nodeHost = group[i][j].HostName;    // HostName

            // Start primary node in the end
            startNode( db, getRG, nodeHost, node );
         }

         if( !result )
         {
            println( "Don't change the primary node, node = " + newPrimNode );
            throw "ErrVotePrimary";
         }
      }
      else
      {
         println( "The nodes less than 3 in group : " + getRG +
            ", cannot be stop." );
      }
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
finally
{

}
