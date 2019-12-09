/************************************
*@Description: 修改固定集合为普通集合，修改参数为开启压缩/AutoSplit/AutoIndexId  
*@author:      liuxiaoxuan
*@createdate:  2017.9.30
*@testlinkCase:seqDB-12803/seqDB-12804
**************************************/

main();

function main ()
{
   if( commIsStandalone( db ) )
   {
      println( 'skip standlone' );
      return;
   }

   //create cappedCL
   var clName = COMMCAPPEDCLNAME + "12803_12804";
   var clOption = { Capped: true, Size: 1024, AutoIndexId: false };
   var dbcl = commCreateCLByOption( db, COMMCAPPEDCSNAME, clName, clOption, false, true );

   //check alter cappedCL 
   println( "---begin check alter cappedCL---" )
   var expectErrorCode32 = -32;
   var alterOption1 = { Capped: false };
   checkAlterResult( dbcl, alterOption1, expectErrorCode32 );
   var alterOption2 = { Compressed: true };
   checkAlterResult( dbcl, alterOption2, expectErrorCode32 );
   var alterOption3 = { AutoIndexId: true };
   checkAlterResult( dbcl, alterOption3, expectErrorCode32 );
   var alterOption4 = { AutoSplit: true };
   checkAlterResult( dbcl, alterOption4, expectErrorCode32 );

   //create commonCS and commonCL
   var commonCLName = COMMCLNAME + "12803_12804";
   var dbcl = commCreateCL( db, COMMCSNAME, commonCLName );

   //check alter commonCL 
   println( "---begin check alter CommonCL---" );
   var alterOption5 = { Capped: true };
   checkAlterResult( dbcl, alterOption5, expectErrorCode32 );
   var alterOption6 = { Size: 1024 };
   checkAlterResult( dbcl, alterOption6, expectErrorCode32 );
   var alterOption7 = { Max: 10000000 };
   checkAlterResult( dbcl, alterOption7, expectErrorCode32 );
   var alterOption8 = { OverWrite: true };
   checkAlterResult( dbcl, alterOption8, expectErrorCode32 );

   println( "---end check---" );
   commDropCL( db, COMMCAPPEDCSNAME, clName, true, true, "drop CL in the end" );
   commDropCL( db, COMMCSNAME, commonCLName, true, true, "drop CL in the end" );
}

function checkAlterResult ( dbcl, options, expectErrorCode )
{
   try
   {
      dbcl.alter( options );
      throw "NEED_ALTER_ERROR";
   }
   catch( e )
   {
      if( expectErrorCode !== e )
      {
         throw buildException( "checkAlterResult()", e, "check alter result", expectErrorCode, e );
      }
   }
}
