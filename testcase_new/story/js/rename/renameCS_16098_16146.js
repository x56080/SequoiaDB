/************************************
*@Description: 多次修改cs名--------//1、用例描述信息和实际不符
*@author:      luweikang
*@createdate:  2018.10.12
*@testlinkCase:seqDB-16098
**************************************/

main();

function main ()
{
   println( "---begin rename cs test---" );
   var csname1 = CHANGEDPREFIX + "_oldcs_16098_16146";
   var csname2 = CHANGEDPREFIX + "_newcs_16098_16146";
   var notExitName = "notExitName_cs";
   var clName = CHANGEDPREFIX + "_cl_16098_16146";

   commCreateCS( db, csname1, false, "create cs in begine", "" );
   commCreateCS( db, csname2, false, "create cs in begine", "" );

   println( "---rename not exits cs---" );
   try
   {
      db.renameCS( notExitName, csname1 );
   }
   catch( e )
   {
      if( e !== -34 )
      {
         throw buildException( "rename not exits cs:", e );
      }
   }

   println( "---rename cs already exits cs---" );
   try
   {
      db.renameCS( csname1, csname2 );
   }
   catch( e )
   {
      if( e !== -33 )
      {
         throw buildException( "rename cs already exits cs:", e );
      }
   }

   commDropCS( db, csname1, true, false, "clean cs---" );
   commDropCS( db, csname2, true, false, "clean cs---" );
   println( "---end the test---" );
}

