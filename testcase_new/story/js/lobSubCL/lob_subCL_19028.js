/************************************
*@Description: seqDB-19028 创建主表，指定LobShardingKeyFirmat和多个分区键
*@author:      luweikang
*@createDate:  2019.8.12
**************************************/
try
{
   main(); 
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack ); 
   }
   throw e; 
}

function main()
{
   if( commIsStandalone( db ) )
   {
      println( "skip standalone mode" ); 
      return; 
   }
   var mainCSName = COMMCSNAME; 
   var mainCLName = "mainCL_19028"; 
   
   var cs = commCreateCS( db, mainCSName, true ); 
   var options = {"IsMainCL": true, "ShardingKey": {"date": 1, "a": 1, "b": 2}, "LobShardingKeyFormat": "YYYYMMDD", "ShardingType": "range"}; 
   try
   {
      println( "---create mainCL use more shardingKey---" ); 
      cs.createCL( mainCLName, options ); 
      throw 0; 
   }
   catch( e )
   {
      if( e !== -6 )
      {
         throw buildException( "create mainCL", e, "create mainCL use more shardingKey: " + mainCLName, -6, e ); 
      }
   }
   
}

