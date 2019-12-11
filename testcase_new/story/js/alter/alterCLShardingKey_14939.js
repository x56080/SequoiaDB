/* *****************************************************************************
@discretion: alter shardingKey field equals to the index field
@author��2018-4-25 wuyan  Init
***************************************************************************** */
var clName = CHANGEDPREFIX + "_alterclShardingKey_14939";

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
      var dbcl = commCreateCLByOption( db, COMMCSNAME, clName, { ShardingKey: { a: 1, b: 1 } } );;

      //create index, then alter shardingKey the same as index field
      var shardingKeyField = { c: 1 };
      dbcl.createIndex( "testindex", shardingKeyField );
      dbcl.enableSharding( { ShardingKey: shardingKeyField } );
      checkAlterResult( clName, "ShardingKey", shardingKeyField );
      checkShardIndex( dbcl );

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

function checkShardIndex ( dbcl )
{
   try
   {
      dbcl.getIndex( "$shard" );
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      //-47:Index name does not exist
      if( e.message != -47 )
      {
         throw e;
      }
   }
}



