/************************************
*@Description: 创建普通集合空间，并在该集合空间上创建固定集合
               创建固定集合空间，并在该集合空间上创建普通集合
*@author:      luweikang
*@createdate:  2017.7.7
*@testlinkCase:seqDB-11836,seqDB-11837
**************************************/

main();

function main ()
{
   var normal_csName = COMMCSNAME + "_11836";
   var capped_csName = COMMCAPPEDCSNAME + "_11837";

   var normal_clName = COMMCLNAME + "_11837";
   var capped_clName = COMMCAPPEDCLNAME + "_11836";

   //drop CS and create cappedCS
   println( "---begin test---" );
   commDropCS( db, normal_csName, true, "drop CS in the beginning" );
   commCreateCS( db, normal_csName, false, "beginning to create CS", null );
   initCappedCS( capped_csName );

   //normalCS create cappedCL
   println( "---normalCS create cappedCL---" )
   normalCScreateCL( normal_csName, capped_clName );

   //cappedCS create normalCL
   println( "---cappedCS create normalCL---" )
   cappedCScreateCL( capped_csName, normal_clName );

   //clean environment after test
   println( "---end the test---" );
   commDropCS( db, normal_csName, true, "drop CS in the end" );
   commDropCS( db, capped_csName, true, "drop CS in the end" );
}

function normalCScreateCL ( normal_csName, capped_csName )
{
   var optionObj = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: false };
   try
   {
      db.getCS( normal_csName ).createCL( capped_csName, optionObj );
      throw "ERR_NORMALCS_CREATE_CAPPEDCL";
   }
   catch( e )
   {
      if( e !== -315 )
      {
         throw buildException( "normalCS create cappedCL", e, "normalCS create cappedCL", "-315", e );
      }
   }
}

function cappedCScreateCL ( capped_csName, normal_clName )
{
   try
   {
      db.getCS( capped_csName ).createCL( normal_clName );
      throw "ERR_CAPPEDCS_CREATECL";
   }
   catch( e )
   {
      if( e !== -315 )
      {
         throw buildException( "cappedCS create normalCL", e, "cappedCS create normalCL", "-315", e );
      }
   }
}