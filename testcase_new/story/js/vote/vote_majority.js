/******************************************************************************
@Description : Test stop majority node in data group and then start it.
@Modify list :
               2014-6-12  xiaojun Hu  Init
******************************************************************************/

function main ( db )
{
   // Get Primary node related infomation
   var group = commGetGroups( db, "", "", true, true ) ;
   var rgSize = group.length ;
   println( " Group Size : " + rgSize ) ;
   for( var i = 0 ; i < rgSize ; ++i )
   {
      var nodeSize = group[i].length ;
      var getRG = group[i][0].GroupName ;         // GroupName
      var primNode = group[i][0].PrimaryNode ;    // PrimaryNode
      // If the nodes less than 3, nodes cannot be stop
      if( 3 < nodeSize )
      {
         // Majority nodes = (nodeSize/2+1)
         for( var j = 1 ; j < Math.floor(nodeSize/2)+1 ; ++j )    //many groups,begin 1 not 0
         {
            var node = group[i][j].svcname ;    // svcname
            var nodeHost = group[i][j].HostName ;   // HostName

            // Stop primary node
            stopNode( db, getRG, nodeHost, node ) ;
         }

         // Inspect the new primary node ant the olde primary node
         var count = 0 ;
         do
         {
            ++ count ;
            var newPrimNode = getPrimNode( db, getRG ) ;
            //println( "node ID" + newPrimNode + " = " + primNode ) ;
            if( 10000 < count )
            {
               println( "Don't change the primary node, node = " + masterNode ) ;
               throw "ErrVotePrimary" ;
            }
            //println( "count : " + count ) ;
         }while( false == newPrimNode ) ;

         for( var j = 1 ; j < Math.floor(nodeSize/2)+1 ; ++j )    //many groups,begin 1 not 0
         {
            var node = group[i][j].svcname ;    // svcname
            var nodeHost = group[i][j].HostName ;    // HostName

            // Start primary node in the end
            startNode( db, getRG, nodeHost, node ) ;
         }

      }
      else
      {
         println( "The nodes less than 3 in group : " + getRG +
                  ", cannot be stop." ) ;
      }
   }
}

// Main Running
try
{
   var mode = commIsStandalone ( db ) ;
   if ( false == mode )
      main ( db ) ;
   else
      println ( "Run Mode is : Standalone" ) ;
}
catch ( e )
{
   throw e ;
}

