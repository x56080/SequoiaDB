/******************************************************************************
@Description : Test the vote the primary library.
@Modify list :
               2014-6-12  xiaojun Hu  Init
******************************************************************************/

var csName = COMMCSNAME ;
var clName = COMMCLNAME ;

// Stop the primary node by using 'node.stop' in Group
function stopNode( db, groupName, primHost, primNode )
{
   try
   {
      var rg = db.getRG ( groupName ) ;
      var node = rg.getNode ( primHost, primNode ) ;
      node.stop() ;   // stop node
      println( "Success to stop Node : [ " + primHost +
               " : " + primNode + " ]" ) ;
   }
   catch( e )
   {
      println( "Failed to stop the primary node = [ " + primHost + primNode +
               " ], rc = " + e ) ;
      throw e ;
   }
}


// Start primary ndoe by using 'node.start' in Group
function startNode( db, groupName, primHost, primNode )
{
   try
   {
      var rg = db.getRG ( groupName ) ;
      var node = rg.getNode ( primHost, primNode ) ;
      node.start() ;   // start node
      println( "Success to start Node : [ " + primHost +
               " : " + primNode + " ]" ) ;
   }
   catch( e )
   {
      println ( "Failed to start the primary node, rc = " + e ) ;
      throw e ;
   }
}

// Get primary node after stopping
function getPrimNode( db, groupName )
{
   try
   {
      /*var group = commGetGroups( db ) ;
      var rgSize = group.length ;
      for( var i = 0 ; i < rgSize ; ++i )
      {
         var getRG = group[i][0].GroupName ;
         //println( "get group = " + getRG + " " + groupName ) ;
         if( groupName == getRG )
         {
            var primNode = group[i][0].PrimaryNode ;
            break ;
         }
      }*/
      
      var nodeName = db.getRG(groupName).getMaster();

      var nodeDb = new Sdb( nodeName );
      return nodeDb.snapshot(6).next().toObj().IsPrimary ;
   }
   catch( e )
   {
      if( -79 != e  && -15 != e && -155 != e)
      {
         println( "Failed to get primary node, rc = " + e ) ;
         throw e ;
      }
      return false ;
   }
}

// Inspect the group have primary or not
function havePrimInGroup( db, groupName )
{
   try
   {
      var domname = groupName + "_inspectPrimary_" + clName ;
      var csname = clName + "_cs_" + groupName ;
      var clname = clName + "_cl_" + groupName ;
      // Drop CS in the beginning
      try
      {
         db.dropCS( csname ) ;
      }
      catch( e )
      {
         if( -34 != e )
         {
            //println( "Failed to drop CS in the beginning, rc = " + e ) ;
            throw e ;
         }
      }
      // Drop Domain in the beginning
      try
      {
         db.dropDomain( domname ) ;
      }
      catch( e )
      {
         if( -214 != e )
         {
            //println( "Failed to drop domain in the beginning, rc = " + e ) ;
            throw e ;
         }
      }
      db.createDomain( domname, [ groupName ] ) ;
      var cs = db.createCS( csname, { "Domain" : domname } ) ;
      var cl = cs.createCL( clname ) ;
      cl.insert( { "testPrim" : "CannotInsertData" } ) ;
      throw "Cannot createCL success" ;
   }
   catch( e )
   {
      if( -104 != e && -79 != e && -71 != e )
      {
         throw e ;
      }
      //else
         //println( "Don't have primary in group : [ " + groupName + " ]"  ) ;
   }
}

function clearGroup( db, groupName )
{
   try
   {
      var domname = groupName + "_inspectPrimary_" + clName ;
      var csname = clName + "_cs_" + groupName ;
      var clname = clName + "_cl_" + groupName ;

      try
      {
         db.dropCS( csname ) ;
      }
      catch( e )
      {
         println( "Failed to drop CS in the end, rc = " + e ) ;
         throw e ;
      }
      try
      {
         db.dropDomain( domname ) ;
      }
      catch( e )
      {
         println( "Failed to drop domain in the end, rc = " + e ) ;
         throw e ;
      }
   }
   catch( e )
   {
      if( -104 != e && -79 != e && -71 != e )
      {
         println( "Failed to clear the group, rc = " + e ) ;
         throw e ;
      }
   }
}
