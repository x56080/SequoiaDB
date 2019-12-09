/******************************************************************************
@Description : 1.Test stop primary node in data group .
               2.Check the group have primary or not .
               3.Then start node that was stop .
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
         for( var j = 1; j < nodeSize; ++j )    //many groups,begin 1 not 0
         {
            var node = group[i][j].svcname;    // svcname
            var nodeId = group[i][j].NodeID;   //NodeID
            if( primNode == nodeId )
            {
               var primHost = group[i][j].HostName;    // HostName
               var masterNode = group[i][j].svcname;   // Master svcname
               //println( "master host : " + masterNode + "::" + primHost ) ;
               // Stop primary node
               stopNode( db, getRG, primHost, masterNode );

               // Inspect the new primary node ant the olde primary node
               var count = 0;
               var totalLen = 60;
               do
               {
                  sleep( 1000 );
                  ++count;
                  var newPrimNode = getPrimNode( db, getRG );
                  //println( "node ID" + newPrimNode + " = " + primNode ) ;
                  if( count >= totalLen )
                  {
                     result = false;
                  }
                  //println( "count : " + count ) ;
               } while( false == newPrimNode );

               // Start primary node in the end
               startNode( db, getRG, primHost, masterNode );
               break;
            }
         }
         if( result )
         {
            break;
         }
      }
      else
      {
         println( "The nodes less than 3 in group : " + getRG +
            ", cannot be stop." );
      }
   }
   if( !result )
   {
      println( "Don't change the primary node, node = " + masterNode );
      throw "ErrVotePrimary";
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

