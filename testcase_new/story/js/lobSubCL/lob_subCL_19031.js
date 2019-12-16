/************************************
*@Description: seqDB-19031 主表挂载子表，设置LowBound和UpBound类型为非字符串
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

function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "skip standalone mode" );
      return;
   }

   var csName = COMMCSNAME;
   var mainCLName = "mainCL_19031";
   var subCLName = "subCL_19031";

   commDropCL( db, csName, mainCLName );
   commDropCL( db, csName, subCLName );

   var options = { "IsMainCL": true, "ShardingKey": { "date": 1 }, "LobShardingKeyFormat": "YYYYMM", "ShardingType": "range" };
   var mainCL = commCreateCL( db, csName, mainCLName, options, true, false, "create main cl" );
   commCreateCL( db, csName, subCLName );

   try
   {
      mainCL.attachCL( csName + "." + subCLName, { "LowBound": { "date": 201901 }, "UpBound": { "date": 201907 } } );
      throw 0;
   }
   catch( e )
   {
      if( e !== -238 )
      {
         throw buildException( "attach cl", e, "attachCL but bound error: " + subCLName, -238, e );
      }
   }

   try
   {
      mainCL.attachCL( csName + "." + subCLName, { "LowBound": { "date": "201901" }, "UpBound": { "date": 201907 } } );
      throw 0;
   }
   catch( e )
   {
      if( e !== -238 )
      {
         throw buildException( "attach cl", e, "attachCL but bound error: " + subCLName, -238, e );
      }
   }

   try
   {
      mainCL.attachCL( csName + "." + subCLName, { "LowBound": { "date": "20190101" }, "UpBound": { "date": "20190701" } } );
      throw 0;
   }
   catch( e )
   {
      if( e !== -238 )
      {
         throw buildException( "attach cl", e, "attachCL but bound error: " + subCLName, -238, e );
      }
   }

   commDropCL( db, csName, mainCLName );
   commDropCL( db, csName, subCLName );
}

