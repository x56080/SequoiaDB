/******************************************************************************
@Description : SEQUOIADBMAINSTREAM-10204：新增接口参数检查
               测试步骤：
               1. OnlyUpgradeMeta 参数指定为数字类型，预期结果：报错 -6
               2. OnlyUpgradeMeta 参数指定为字符串类型，预期结果：报错 -6
               3. OnlyUpgradeMeta 参数指定为布尔类型 true，预期结果：不报错
               4. OnlyUpgradeMeta 参数指定为布尔类型 false，预期结果：不报错
               5. 以上步骤分别测试普通表，分区表和主子表
@Modify list :
               2025-02-19 fangjiabin  Init
******************************************************************************/
main( test ) ;

function test ()
{
   var csName = "args_check_10204_cs" ;
   var clName1 = "args_check_10204_1_cl" ;
   var clName2 = "args_check_10204_2_cl" ;
   var mainCL_Name = "args_check_10204_main_cl" ;
   var subCL_Name = "args_check_10204_sub_cl" ;

   commDropCS( db, csName ) ;

   var cl1 = commCreateCL( db, csName, clName1, { "AutoIndexId": false } ) ;
   var cl2 = commCreateCL( db, csName, clName2, { "AutoIndexId": false, "AutoSplit": true, "ShardingKey": { "a": 1 } } ) ;
   var mainCLOption = { ShardingKey: { "a": 1 }, IsMainCL: true } ;
   var maincl = commCreateCL( db, csName, mainCL_Name, mainCLOption, true, true ) ;
   var subClOption = { ShardingKey: { "b": 1 }, AutoSplit: true, ReplSize: 0 } ;
   commCreateCL( db, csName, subCL_Name, subClOption, true, true ) ;
   var options = { LowBound: { a: 1 }, UpBound: { a: 100 } } ;
   maincl.attachCL( csName + "." + subCL_Name, options ) ;

   // 普通表
   executeCheck( cl1 ) ;
   // 分区表
   executeCheck( cl2 ) ;
   // 主子表
   executeCheck( maincl ) ;
}

function executeCheck( cl )
{
   checkCreateIndexArgs( cl, 128, "Number", true ) ;
   checkCreateIndexArgs( cl, "128", "String", true ) ;
   checkCreateIndexArgs( cl, true, "Boolean", false ) ;
   checkCreateIndexArgs( cl, false, "Boolean", false ) ;

   checkCreateIdIndexArgs( cl, 128, "Number", true ) ;
   checkCreateIdIndexArgs( cl, "128", "String", true ) ;
   checkCreateIdIndexArgs( cl, true, "Boolean", false ) ;
   checkCreateIdIndexArgs( cl, false, "Boolean", false ) ;
}

function checkCreateIndexArgs( cl, onlyUpgradeMeta, argsType, isErrArgs )
{
   var catchArgsErr = false ;
   try
   {
      cl.createIndex( "aIdx", { "a": 1 }, { "Unique": true }, { "OnlyUpgradeMeta": onlyUpgradeMeta } ) ;
   }
   catch( e )
   {
      if( e == -6 )
      {
         catchArgsErr = true ;
      }
   }

   if ( ( isErrArgs && !catchArgsErr ) || ( !isErrArgs && catchArgsErr ) )
   {
      throw new Error( "Invalid createIndex args(" + argsType + ":" + onlyUpgradeMeta + "), we should catch -6 error" ) ;
   }
}

function checkCreateIdIndexArgs( cl, onlyUpgradeMeta, argsType, isErrArgs )
{
   var catchArgsErr = false ;
   try
   {
      cl.createIdIndex( { "OnlyUpgradeMeta": onlyUpgradeMeta } ) ;
   }
   catch( e )
   {
      if( e == -6 )
      {
         catchArgsErr = true ;
      }
   }

   if ( ( isErrArgs && !catchArgsErr ) || ( !isErrArgs && catchArgsErr ) )
   {
      throw new Error( "Invalid createIndex args(" + argsType + ":" + onlyUpgradeMeta + "), we should catch -6 error" ) ;
   }
}
