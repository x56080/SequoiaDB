/* *****************************************************************************
@discretion: cl alter partition
@author£º2018-4-25 wuyan  Init
***************************************************************************** */
var clName = CHANGEDPREFIX + "_alterclpartition_14955"; 

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
      
      //test a :alter parition, no shardingKey
      alterParitionNoShardingKey( dbcl ); 
      
      //test b: alter parition and shardingKey
      var shardingKey = {a:1, b:1}; 
      var parition = 2048; 
      dbcl.setAttributes( {ShardingKey:shardingKey, Partition:parition} ); 
      checkAlterResult( clName, "ShardingKey", shardingKey ); 
      checkAlterResult( clName, "Partition", parition ); 
      
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

function alterParitionNoShardingKey( dbcl )
{
   try
   {
      dbcl.setAttributes( {Paritition:512} ); 
      throw "need throw error"; 
   }
   catch( e )
   {
      if( e != -6 )
      {
         throw buildException( "cannot be alter, fail:", e ); 
      }
   }
}

