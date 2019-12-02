/************************************
*@Description: alter修改分区属性
*@author:      luweikang
*@createdate:  2018.4.25
*@testlinkCase:seqDB-15032
**************************************/

main(); 

function main()
{
   if( commIsStandalone( db ) )
   {
      println( "Run mode is standalone" ); 
      return; 
   }
   println( "---begin test---" ); 
   var csName = COMMCSNAME; 
   var clName = CHANGEDPREFIX + "_15032"; 
   
   var cl = commCreateCL( db, csName, clName, 1, false, true, false, "create CL in the begin" ); 
   for( i = 0; i < 5000; i++ )
   {
      cl.insert( {a:i, b:"sequoiadh test split cl alter option"} ); 
   }
   
   //这个地方写测试步骤
   println( "---test alter 1---" ); 
   cl.alter( {ShardingKey: {a:1}, ShardingType: "hash", Partition: 1024, EnsureShardingIndex: true, AutoSplit: false} ); 
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingKey", {a: 1} ); 
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingType", "hash" ); 
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "Partition", 1024 ); 
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "EnsureShardingIndex", true ); 
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "AutoSplit", false ); 
   
   println( "---test alter 2---" ); 
   cl.alter( { ShardingKey: {a:1, b:1}, ShardingType: "range", EnsureShardingIndex: false } ); 
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingKey", {a:1, b:1} ); 
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingType", "range" ); 
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "EnsureShardingIndex", false ); 
   
   println( "---test alter 3---" ); 
   try
   {
      cl.alter( {ShardingKey:{b:1}, ShardingType:"hash", IsMainCL:true, "AutoSplit": true, Compressed: false, lobPageSize:8192} ); 
   }
   catch( e )
   {
      if( e !== -6 )
      {
         throw buildException( "test setAttributes", e, "value is wrong", -6, e ); 
      }
   }
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingKey", {a:1, b:1} ); 
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingType", "range" ); 
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "IsMainCL", undefined ); 
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "AutoSplit", false ); 
   
   commDropCL( db, csName, clName, true, false, "clean cl" ); 
   println( "---end the test---" ); 
}
