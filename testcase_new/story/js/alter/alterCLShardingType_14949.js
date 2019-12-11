/* *****************************************************************************
@discretion: parition cl alter shardingType, the test scenario is as follows:
test a: alter shardingType from range to hash
test b: alter shardingType from hash to range
@author��2018-4-25 wuyan  Init
***************************************************************************** */
var clName = CHANGEDPREFIX + "_alterclShardingType_14949";

try
{
   main( db );
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw e;
}

function main ( db )
{
   try
   {
      if( true == commIsStandalone( db ) )
      {
         println( "run mode is standalone" );
         return;
      }
      //clean environment before test
      commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

      //create cl
      var dbcl = commCreateCLByOption( db, COMMCSNAME, clName, { ShardingKey: { a: 1, b: 1 }, ShardingType: "range" } );;

      //test a: alter shardingType from range to hash
      var shardingType = "range";
      dbcl.enableSharding( { ShardingKey: { a: 1, b: 1 }, ShardingType: shardingType } );
      checkAlterResult( clName, "ShardingType", shardingType );

      //test b: alter shardingType from hash to range
      var shardingType1 = "hash";
      dbcl.enableSharding( { ShardingKey: { a: 1, b: 1 }, ShardingType: shardingType1 } );
      checkAlterResult( clName, "ShardingType", shardingType1 );

      //clean
      commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" );
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      if( db != null )
      {
         db.close()
      }
   }
}

function checkResult ( clName, fieldName, expFieldValue )
{
   var clFullName = COMMCSNAME + "." + clName;
   var cur = db.snapshot( 8, { "Name": clFullName } );
   var actualFieldValue = cur.current().toObj()[fieldName];
   if( typeof ( expFieldValue ) === "object" )
   {
      if( JSON.stringify( expFieldValue ) !== JSON.stringify( actualFieldValue ) )
      {
         throw buildException( "test fieldvalue", "check field", "value is wrong", JSON.stringify( expFieldValue ), JSON.stringify( actualFieldValue ) );
      }

   }
   else
   {
      if( expFieldValue !== actualFieldValue )
      {
         throw buildException( "test fieldvalue", "check field", "value is wrong", expFieldValue, actualFieldValue );
      }
   }
}



