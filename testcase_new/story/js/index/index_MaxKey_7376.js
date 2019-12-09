/***************************************************************************
@Description :create a index which tne key is more than 1000B
@Modify list :
              2014-5-21  xiaojun Hu  Init
              2016-3-4   yan wu Modify(增加预置条件和结果检测（插入字段对应的values的大小超过1000b，检查创建索引结果)
****************************************************************************/

try
{
   commDropCL( db, csName, clName, true, true, "drop cl in the beginning" );
}
catch( e )
{
   println( "unexpected err happened when clear cs:" + e );
   throw e;
}

try
{
   var optionObj = { ReplSize: 0, Compressed: true };
   var varCL = commCreateCLByOption( db, csName, clName, optionObj, true,
      false, "create collecton 1 failed" );
} catch( e )
{
   throw e;
}

try
{
   varCL.insert( { a: 1, longint: 2147483647000, floatNum: 12345.456 } );
   varCL.insert( { a: 2, b: "abcdgasdgasdgadgadgadgasdgadsgasdgadgasdgasdgasdgasdgasdgetetetetetetetasdgasdgasdgasdgasdgasdgasdgadgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdggasdgasdgasdgasdgadgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdggasdgasdgasdgasdgadgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdggasdgasdgasdgasdgadgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdggasdgasdgasdgasdgadgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdggadgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdggasdgasdgasdgasdgadgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdggadgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdggasdgasdgasdgasdgadgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdggadgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdggasdgasdgasdgasdgadgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdggadgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdggasdgasdgasdgasdgadgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdggasdgasdgasdgasdgadgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdggadgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdggas12" } );
   createIndex( varCL, "testindex", { b: 1 }, false, false, -39 );
}
catch( e )
{
   println( "failed to create index" );
   throw e;
}

try
{
   commDropCL( db, csName, clName, false, false,
      "drop colleciton in the end" );
}
catch( e )
{
   println( "unexpected err happened when clear cs:" + e );
   throw e;
}

