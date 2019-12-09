/************************************
*@Description: 内置 SQL 中 $SNAPSHOT_CONFIGS 使用 with_option
*@author:      chensiqin
*@createdate:  2019.06.13
*@testlinkCase: seqDB-18431
**************************************/

main();

function main ()
{
   if( commGetGroupsNum( db ) < 1 )
   {
      return;
   }
   var groupNames = getGroup( db );

   var option = new SdbSnapshotOption().cond( { GroupName: groupNames[0] } ).sort( { NodeName: 1 } ).options( { "Mode": "run", "Expand": false } );
   var expectedInfo = db.snapshot( SDB_SNAP_CONFIGS, option );
   var actualInfo = db.exec( 'select * from $SNAPSHOT_CONFIGS where GroupName = "' + groupNames[0] + '" order by NodeName asc /*+use_option(Mode, run) use_option(Expand, false)*/' );
   checkRec( actualInfo, expectedInfo );

   var option = new SdbSnapshotOption().cond( { GroupName: groupNames[0] } ).sort( { NodeName: 1 } ).options( { "Mode": "local", "Expand": false } );
   var expectedInfo = db.snapshot( SDB_SNAP_CONFIGS, option );
   var actualInfo = db.exec( 'select * from $SNAPSHOT_CONFIGS where GroupName = "' + groupNames[0] + '" order by NodeName asc /*+use_option(Mode, local) use_option(Expand, false)*/' );
   checkRec( actualInfo, expectedInfo );
}

function getGroup ( db )
{
   try
   {
      var listGroups = db.listReplicaGroups();
      var groupArray = new Array();
      while( listGroups.next() )
      {
         if( listGroups.current().toObj()["GroupID"] >= DATA_GROUP_ID_BEGIN )
         {
            groupArray.push( listGroups.current().toObj()["GroupName"] );
         }
      }
      return groupArray;
   }
   catch( e )
   {
      println( "Failed to get groups from sdb, rc = " + e );
      throw e;
   }
}

function checkRec ( actualRc, expectedRc )
{
   println( "---begin to check data context." );
   //get actual records to array
   var actRecs = [];
   var expRecs = [];

   while( actualRc.next() )
   {
      actRecs.push( actualRc.current().toObj() );
   }

   while( expectedRc.next() )
   {
      expRecs.push( expectedRc.current().toObj() );
   }

   //check count
   if( actRecs.length !== expRecs.length )
   {
      throw buildException( "check count", expRecs.length, "",
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

}