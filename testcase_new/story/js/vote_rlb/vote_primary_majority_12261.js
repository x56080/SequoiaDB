/******************************************************************************
@Description : Test stop minority nodes and the minority nodes have
               primary node in the group.
@Modify list :
               2014-6-12  xiaojun Hu  Init
******************************************************************************/

function main ( db )
{
   // Get Primary node related infomation
   var group = commGetGroups( db, "", "", true, true ) ;
   var rgSize = group.length ;
   var isTest = false;
   println( " Group Size : " + rgSize ) ;
   for( var i = 0 ; i < rgSize ; ++i )
   {
      var nodeSize = group[i].length - 1;
      var getRG = group[i][0].GroupName ;         // GroupName
      var primNode = group[i][0].PrimaryNode ;    // PrimaryNode
      var result = true;
      // If the nodes less than 3, nodes cannot be stop
      if( !isTest && nodeSize >= 3 )
      {
         println( "***Start to inspect the voting in group = [" + getRG + " ]***" ) ;
         var mino = 1 ; // contain primary node
         // 1.Stop majority nodes
         for( var j = 0 ; j < nodeSize ; ++j )    //many groups,begin 1 not 0
         {
            var nodeID = group[i][j+1].NodeID ;    // NodeID
            var node = group[i][j+1].svcname ;    // svcname
            var nodeHost = group[i][j+1].HostName ;    // HostName
            if( primNode == nodeID )
            {
               var primHost = nodeHost ;
               var master = node ;    // Master node
               continue ;
            }
            if( mino < Math.floor(nodeSize/2)+1 )
            {
               // Stop no primary node
               stopNode( db, getRG, nodeHost, node ) ;
               mino++;
            }
         }
         // 2.Stop primary node
         stopNode( db, getRG, primHost, master ) ;

         // 3.Inspect the new primary node ant the olde primary node
         try
         {
            havePrimInGroup( db, getRG ) ;
         }
         catch( e )
         {
            if( -79 == e )
               println( "Don't have primary in group : [ " + groupName + " ]"  ) ;
            else
               throw e ;
         }
         println( "Inspect Over" ) ;

         // 4.Start primary node
         startNode( db, getRG, primHost, master ) ;

         // 5.Start majority nodes
         for( var j = 0 ; j < Math.floor(nodeSize/2)+1 ; ++j )    //many groups,begin 1 not 0
         {
            var node = group[i][j+1].svcname ;    // svcname
            var nodeHost = group[i][j+1].HostName ;    // HostName

            // Start primary node in the end
            startNode( db, getRG, nodeHost, node ) ;
         }

         // Inspect the new primary node ant the olde primary node
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
         
         isTest = true ;
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
