/******************************************************************************
@Description : Test stop group, then start minority node , then inspect primary,
               then start majoryty nodes, then inspect primary in the end.
@Modify list :
               2014-6-12  xiaojun Hu  Init
******************************************************************************/

function main ( db )
{
   // Get Primary node related infomation
   var group = commGetGroups( db, "", "", true, true );
   var rgSize = group.length;
   println( " Group Size : " + rgSize );
   for( var i = 0; i < rgSize; ++i )
   {
      var nodeSize = group[i].length;
      var getRG = group[i][0].GroupName;         // GroupName
      var primNode = group[i][0].PrimaryNode;    // PrimaryNode
      // If the nodes less than 3, nodes cannot be stop.Catalog group cannot do
      //println( "node SIZE = " + nodeSize ) ;
      if( 3 < nodeSize )
      {
         println( "***Start to inspect the voting in group = [" + getRG + " ]***" );
         // Start the group in the beginning
         try
         {
            var rg = db.getRG( getRG );
            rg.start();
         }
         catch( e )
         {
            println( "Failed to start group in the beginning, group = " + getRG );
            throw e;
         }
         sleep( 1000 );
         // 1.Stop the group
         try
         {
            var rg = db.getRG( getRG );
            rg.stop();
         }
         catch( e )
         {
            println( "Failed to stop group, group = " + getRG );
            throw e;
         }

         // 2.Start minority nodes in group
         for( var j = 1; j < Math.floor( nodeSize / 2 ); ++j )    //many groups,begin 1 not 0
         {
            var node = group[i][j].svcname;    // svcname
            var nodeHost = group[i][j].HostName;    // HostName

            // Start primary node in the end
            startNode( db, getRG, nodeHost, node );
         }

         // 3.Inspect the group don't have primary
         try
         {
            havePrimInGroup( db, getRG );
         }
         catch( e )
         {
            if( -79 == e )
               println( "Don't have primary in group : [ " + groupName + " ]" );
            else
               throw e;
         }

         // 4.Start the majority nodes
         println( "Inspect Over" );
         for( var j = Math.floor( nodeSize / 2 ); j < nodeSize; ++j )    //many groups,begin 1 not 0
         {
            var node = group[i][j].svcname;    // svcname
            var nodeHost = group[i][j].HostName;    // HostName

            // Start primary node in the end
            startNode( db, getRG, nodeHost, node );
         }

         // 5.Inspect the Group than have primary
         // Inspect the new primary node ant the olde primary node
         var majCount = 0;
         var totalTimeLen = 60;
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
               else
               {
                  println( "Start majority nodes, then have Primary node" );
                  clearGroup( db, getRG );
                  break;
               }
            }
            ++majCount;
         } while( totalTimeLen > majCount );
         if( totalTimeLen == majCount )
            throw "Don't have primary node in the end";
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
