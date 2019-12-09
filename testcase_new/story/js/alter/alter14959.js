/************************************
*@Description: 修改IsMainCL主分区信息
*@author:      luweikang
*@createdate:  2018.4.26
*@testlinkCase:seqDB-14959
**************************************/

main();

function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "Run mode is standalone" );
      return;
   }

   println( "---begin test---" );
   var csName = COMMCSNAME;
   var mainCLName = CHANGEDPREFIX + "_main_14959";
   var clName = CHANGEDPREFIX + "_14959";

   var options = { IsMainCL: true, ShardingType: 'range', ShardingKey: { a: 1 } };
   var mainCL = commCreateCLByOption( db, csName, mainCLName, options, true, false, "create CL in the begin" );
   var cl = commCreateCL( db, csName, clName, 1, false, true, false, "create CL in the begin" );

   //修改IsMainCL值
   println( "---test alter IsMainCL---" );
   alterIsMainCL( mainCL, { IsMainCL: false } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, mainCLName, "IsMainCL", true );

   alterIsMainCL( cl, { IsMainCL: true, ShardingType: 'range', ShardingKey: { 'a': 1 } } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "IsMainCL", undefined );

   commDropCL( db, csName, mainCLName, true, false, "clean cl" );
   commDropCL( db, csName, clName, true, false, "clean cl" );
   println( "---end the test---" );
}

//修改IsMainCL报-6，原本就是这样定义的，所以不改动
function alterIsMainCL ( cl, alterOption )
{
   try
   {
      cl.setAttributes( alterOption );
      throw "ALTER_SHOULD_ERR";
   }
   catch( e )
   {
      if( e !== -6 )
      {
         throw e;
      }
   }
}
