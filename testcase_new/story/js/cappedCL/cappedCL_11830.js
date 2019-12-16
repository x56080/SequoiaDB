/************************************
*@Description: 创建固定集合，并创建索引
*@author:      luweikang
*@createdate:  2017.7.7
*@testlinkCase:seqDB-11830
**************************************/

main();

function main ()
{
   //create cappedCL
   var clName = COMMCAPPEDCLNAME + "_11830";
   var optionObj = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: false };
   var dbcl = commCreateCL( db, COMMCAPPEDCSNAME, clName, optionObj, false, false, "create cappedCL" );

   //createIndex
   println( "---createIndex---" )
   try
   {
      dbcl.createIndex( "ageIndex", { age: 1 }, true );
   }
   catch( e )
   {
      if( e !== -32 )
      {
         throw buildException( "cappedCL createIndex", e, "cappedCL createIndex", "-32", e );
      }
   }

   //clean environment after test
   commDropCL( db, COMMCAPPEDCSNAME, clName, true, true, "drop CL in the end" );
   println( "---end the test---" );
}