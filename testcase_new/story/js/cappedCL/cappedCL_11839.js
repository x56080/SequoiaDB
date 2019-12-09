/************************************
*@Description: 创建普通集合空间，并创建与该集合空间名同名的固定集合空间
*@author:      luweikang
*@createdate:  2017.7.7
*@testlinkCase:seqDB-11839
**************************************/

main();

function main ()
{
   var csName = COMMCSNAME + "_11839";

   //drop CS and create cappedCS
   println( "---begin test---" );
   commDropCS( db, csName, true, "drop CS in the beginning" );

   //create normal CS
   println( "---create normal CS---" )
   commCreateCS( db, csName, false, "beginning to create CS", null );

   //create capped CS
   println( "---create capped CS---" )
   var optionObj = { Capped: true };
   try
   {
      db.createCS( csName, optionObj );
      throw "ERR_CREATE_CAPPEDCS";
   }
   catch( e )
   {
      if( e !== -33 )
      {
         throw buildException( "create capped CS", e, "create capped CS", "-33", e );
      }
   }

   //clean environment after test
   commDropCS( db, csName, true, "drop CS in the end" );
   println( "---end the test---" );
}