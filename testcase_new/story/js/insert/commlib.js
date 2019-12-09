/* *****************************************************************************
@discretion: Insert common functions
@modify list:
   2014-3-1 Jianhui Xu  Init
***************************************************************************** */

var csName = COMMCSNAME;
var clName = COMMCLNAME;

// common functions
function insertAndCheck ( cl, num, removeAll, needCheck, message )
{
   if( undefined == num ) { num = 1; }
   if( undefined == message ) { message = "" }
   if( undefined == needCheck ) { needCheck = true; }
   if( undefined == removeAll ) { removeAll = true; }

   // remove first
   if( removeAll )
   {
      try
      {
         cl.remove();
      }
      catch( e )
      {
         println( "Remove records failed: " + e + ", message: " + message );
         throw e;
      }
   }
   // insert
   try
   {
      for( var i = 0; i < num; ++i )
      {
         cl.insert( { "no": i, "b": 2 * i, "c": false, "d": { "da": [i, i + 1, "oo"], "db": { "dba": "test" } } } );
      }
   }
   catch( e )
   {
      println( "Insert " + i + " records failed: " + e + ", message: " + message );
      throw e;
   }

   if( !needCheck )
   {
      return;
   }
   // check1 - by all
   var size = 0;
   try
   {
      size = cl.count();
      if( size != num )
      {
         println( "Count all size: " + size + " is not same with " + num + ", message: " + message );
         throw "count all check failed"
      }
   }
   catch( e )
   {
      println( "Count all exception: " + e + ", message: " + message );
      throw e;
   }
   // check2 - by find
   // var rc ;
   // try
   // {
   //    size = 0 ;
   //    rc = cl.find( { "no": {"$gte": 0, "$lt": num } } ) ;
   //    while ( rc.next() )
   //    {
   //       ++size ;
   //    }
   //    if ( size != num )
   //    {
   //       println( "Count find size: " + size + " is not same with " + num + ", message: " + message ) ;
   //       throw "count find check failed"
   //    }
   // }
   // catch( e )
   // {
   //    println( "Count find exception: " + e + ", message: " + message ) ;
   //    throw e ;
   // }

   //check3 - verify record
   try
   {
      for( var i = 0; i < num; ++i )
      {
         var lobj = { "no": i, "b": 2 * i, "c": false, "d": { "da": [i, i + 1, "oo"], "db": { "dba": "test" } } };
         var robj = cl.find( { "no": i } ).next().toObj();
         if( !compareObj( lobj, robj ) )
            throw "insert value and find value doesn't match"
      }
   }
   catch( e )
   {
      println( "verify " + i + " records failed: " + e + ", message: " + message );
      throw e;
   }
}

/****************************************************
@description: check the result of query
@modify list:
              2016-3-3 yan WU init
****************************************************/
function checkRec ( rc, expRecs )
{
   println( "---begin to check data context." );
   //get actual records to array
   var actRecs = [];

   while( rc.next() )
   {
      actRecs.push( rc.current().toObj() );
   }

   //check count
   if( actRecs.length !== expRecs.length )
   {
      throw buildException( "check count", null, "",
         expRecs.length, actRecs.length );
   }

   //check every records every fields
   for( var i in expRecs )
   {
      var actRec = actRecs[i];
      var expRec = expRecs[i];
      for( var f in expRec )
      {
         if( !compareObj( actRec[f], expRec[f] ) )
         {
            println( "\nerror occurs in " + ( parseInt( i ) + 1 ) + "th record, in field '" + f + "'" );
            println( "\nactual recs in cl= " + JSON.stringify( actRecs[i] ) + "\n\nexpect recs= " + JSON.stringify( expRecs[i] ) );

            throw buildException( "checkRec()", "check actRecs fail!" );
         }
      }
   }

}

//compare two basic data type value or json object.
function compareObj ( lobj, robj )
{
   if( typeof ( lobj ) === "object" && typeof ( robj ) === "object" )
   {
      for( key in lobj )
      {
         if( undefined === robj[key] ) return false;
         if( !compareObj( lobj[key], robj[key] ) ) return false;
      }
      return true;
   }
   else if( lobj === robj )
   {
      return true;
   }
   else
      return false;
}

// Get the group1 and group2. Position is mean the group in array you specify
// it's location
function getTwoGroupSplit ( db, csName, clName, splitArg1, splitArg2 )
{
   try
   {
      // get collection
      var cs = db.getCS( csName );
      var cl = cs.getCL( clName );

      var listGroups = db.listReplicaGroups();
      var listGroupsArr = new Array();

      // Check over arguement "splitArg1" "splitArg2"
      if( "" == splitArg1 || undefined == splitArg1 )
      {
         println( "Wrong argument." );
         throw "ErrArg";
      }

      // argument : when the split is percent
      var argument = "";
      if( undefined == splitArg2 || "" == splitArg2 ) { argument = splitArg1; }
      // Get group where Collection Space located in
      while( listGroups.next() )
      {
         if( listGroups.current().toObj()["GroupID"] >= DATA_GROUP_ID_BEGIN )
         {
            listGroupsArr.push( listGroups.current().toObj()["GroupName"] );
         }
      }
      var groupNum = listGroupsArr.length;

      var snapShotCS = db.snapshot( SDB_SNAP_COLLECTIONSPACES );
      var snapShotCsName = new Array();
      var snapShotCsGroup = new Array();
      var group = "";
      while( snapShotCS.next() )
      {
         snapShotCsName.push( snapShotCS.current().toObj()["Name"] );
         snapShotCsGroup.push( snapShotCS.current().toObj()["Group"] );
      }
      for( var i = 0; i < snapShotCsGroup.length; i++ )
      {
         if( snapShotCsName[i] == csName )
         {
            group = snapShotCsGroup[i];
            break;
         }
      }
      if( "" == group )
      {
         println( "Failed to get Group where CS located in, snapshotCS = "
            + snapShotCS );
         throw "ErrGetGroup";
      }
      println( "The source group = " + group );
      // Get the other group where split to
      var groupSplit = "";
      var i = 0;
      do
      {
         if( group != listGroupsArr[i] )
         {
            groupSplit = listGroupsArr[i];
            break;
         }
         ++i;

      } while( i <= groupNum || i <= 8 );
      if( "" == groupSplit )
      {
         println( "Failed to get Split Group, Groups = " + listGroups );
         throw "ErrGetSplitGroup";
      }
      println( "The destination [split]group = " + groupSplit );
      //println( argument ) ;
      if( "" == argument )
         cl.split( group, groupSplit, splitArg1, splitArg2 );
      else
         cl.split( group, groupSplit, argument );
      println( "Success to Split" );

   }
   catch( e )
   {
      println( "Failed to get the group " + e );
      throw e;
   }
}

function readyCL ( clName )
{
   println( "\n---Begin to create CL." );

   commDropCL( db, COMMCSNAME, clName, true, true,
      "Failed to drop CL in the pre-condition." );

   var cl = commCreateCL( db, COMMCSNAME, clName, -1, true, true, false,
      "Failed to create CL." );
   return cl;
}

function cleanCL ( clName )
{
   println( "\n---Begin to drop CL." );

   commDropCL( db, COMMCSNAME, clName, false, false,
      "Failed to drop CL in the end-condition" );
}