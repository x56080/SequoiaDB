/******************************************************************************
@Description : Test stop minority nodes and the minority nodes have
               primary node in the group.
@Modify list :
               2014-6-12  xiaojun Hu  Init
******************************************************************************/

function main ( db )
{
   // Get Primary node related infomation
   var group = commGetGroups( db ) ;
   var rgSize = group.length ;
   var isTest = false;
   println( " Group Size : " + rgSize ) ;
   for( var i = 0 ; i < rgSize ; ++i )
   {
      var nodeSize = group[i].length - 1;
      var getRG = group[i][0].GroupName ;         // GroupName
      var primNode = group[i][0].PrimaryNode ;    // PrimaryNode
      var result = true;
      var primHost = "" ;
      var master = "" ;
      // If the nodes less than 3, nodes cannot be stop
      if( !isTest && nodeSize >= 3 )
      {
         var mino = 1 ; // contain primary node 
         println( "node size : " + nodeSize ) ;
         for( var j = 0 ; j < nodeSize ; ++j )    //many groups,begin 1 not 0
         {
            
            var nodeID = group[i][j + 1].NodeID ;    // NodeID
            var node = group[i][j + 1].svcname ;    // svcname
            var nodeHost = group[i][j + 1].HostName ;    // HostName
            if( primNode == nodeID )
            {
               primHost = nodeHost ;
               master = node ;    // Master node
               continue ;
            }
            if( mino < Math.floor(nodeSize/2) - 1 )
            {
               // Stop no primary node
               println( "begin, node? : " + node ) ;
               stopNode( db, getRG, nodeHost, node ) ;
               ++ mino ;
            }
         }
         // Stop primary node
         stopNode( db, getRG, primHost, master ) ;


         // Inspect the new primary node ant the olde primary node
         var count = 0 ;
         var totalSleepLen = 60;
         do
         {
            ++ count ;
            sleep(1000);
            var newPrimNode = getPrimNode( db, getRG ) ;
            //println( "node ID" + newPrimNode + " = " + primNode ) ;
            if( totalSleepLen < count )
            {
               result = false ;
               break;
            }
            //println( "count : " + count ) ;
         }while( false == newPrimNode ) ;

         // Start primary node
         println("start " + primHost + ":" + master);
         startNode( db, getRG, primHost, master ) ;

         for( var j = 0 ; j < Math.floor(nodeSize/2) - 1 ; ++j )    //many groups,begin 1 not 0
         {
            var node = group[i][j+1].svcname ;    // svcname
            var nodeHost = group[i][j+1].HostName ;    // HostName

            // Start primary node in the end
            startNode( db, getRG, nodeHost, node ) ;
         }
         
         
         if ( !result )
         {
            println( "Don't change the primary node, node = " + newPrimNode ) ;
            throw "ErrVotePrimary" ;
         }
         
         var majCount = 0 ;
         var sleepTimeLen = 60;
         do
         {
            sleep(1000);
            try
            {
               havePrimInGroup( db, getRG ) ;
            }
            catch( e )
            {
               if( "Cannot createCL success" != e )
                  throw e ;
               else
               {
                  println( "Start majority nodes, then have Primary node" ) ;
                  clearGroup( db, getRG ) ;
                  break ;
               }
            }
            ++ majCount ;
         }while( sleepTimeLen > majCount ) ;
         if( sleepTimeLen == majCount )
            throw "Don't have primary node in the end" ;
         
         isTest = true;
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
finally
{
   
}
