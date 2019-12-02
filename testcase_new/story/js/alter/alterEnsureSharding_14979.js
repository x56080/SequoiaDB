/* *****************************************************************************
@discretion: cl alter ensureShardingIndex, the test scenario is as follows:
test a: add ensureShardingIndex
test b: add ensureShardingIndex and ShardingKey
@author£º2018-4-25 wuyan  Init
***************************************************************************** */
var clName = CHANGEDPREFIX + "_altercl_14979"; 

main( db ); 
function main( db )
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
      var dbcl = commCreateCL( db, COMMCSNAME, clName ); 
      
      //test a: alter ensureShardingIndex, and no shardingKey
      println( "---test a:alter ensureShardingIndex, and no shardingKey" ); 
      alterEnsureShardingNoShardingKey( dbcl ); 
      
      //test b: alter ensureShardingIndex, and add shardingKey
      println( "---test b:alter ensureShardingIndex and shardingKey" ); 
      var ensureShardingIndex = true; 
      dbcl.setAttributes( {EnsureShardingIndex:ensureShardingIndex, ShardingKey:{a:1}} ); 
      checkAlterResult( clName, "EnsureShardingIndex", ensureShardingIndex ); 
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

function alterEnsureShardingNoShardingKey( dbcl )
{
   try
   {
      dbcl.setAttributes( ( {EnsureShardingIndex:true} ) ); 
      throw "need throw error"; 
   }
   catch( e )
   {
      if( e != -245 )
      {
         throw buildException( "no shardingKey alter should be fail:", e ); 
      }
   }
}

function checkShardIndex( cl )
{
   var timeout = 10; 
   var time = 0; 
   getIndex = cl.getIndex( "$shard" ); 
   while( undefined == getIndex && time < timeout )
   {
      getIndex = cl.getIndex( indexName ); 
++time; 
      sleep( 1000 ); 
   }
   
   var indexDef = getIndex.toObj()["IndexDef"]; 
   var expKeyValue = {a:1}; 
   var name = "$shard"; 
   if( name !== indexDef.name )
   {
      throw buildException( "test index", "check name", "", "", indexDef.name ); 
   }
   if( JSON.stringify( expKeyValue )!== JSON.stringify( indexDef.key ) )
   {
      throw buildException( "test index", "check indexkey", "", "", JSON.stringify( indexDef.key ) ); 
   }
}



