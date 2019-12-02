/************************************
*@Description: 普通表修改ShardingKey
*@author:      luweikang
*@createdate:  2018.4.25
*@testlinkCase:seqDB-14948
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
   var clName = CHANGEDPREFIX + "_14948"; 
   
   var cl = commCreateCL( db, csName, clName, 1, false, true, false, "create CL in the begin" ); 
   
   println( "---alter ShardingType---" ); 
   //only alter ShardingType
   try
   {
      cl.setAttributes( {ShardingType:'range'} ); 
   }
   catch( e )
   {
      if( e !== -245 )
      {
         throw e; 
      }
   }
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingKey", undefined ); 
   
   println( "---alter ShardingType and ShardingKey---" ); 
   //alter ShardingKey and ShardingType
   cl.setAttributes( {ShardingType:'range', ShardingKey:{a :1}} ); 
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingKey", {a: 1} ); 
   
   commDropCL( db, csName, clName, true, false, "clean cl" ); 
   println( "---end the test---" ); 
}
