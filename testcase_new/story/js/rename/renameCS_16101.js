/************************************
*@Description: 修改cs名后，创建、删除cl
*@author:      luweikang
*@createdate:  2018.10.12
*@testlinkCase:seqDB-16101
**************************************/

main();

function main ()
{
   var oldcsName = CHANGEDPREFIX + "_16101_oldcs";
   var newcsName = CHANGEDPREFIX + "_16101_newcs";
   var clName1 = CHANGEDPREFIX + "_16101_cl1";
   var clName2 = CHANGEDPREFIX + "_16101_cl2";

   var cs = commCreateCS( db, oldcsName, false, "create cs in begine", "" );
   var cl = commCreateCL( db, oldcsName, clName1, {}, false, false, "create CL in the begin" );

   println( "---insert 1000 record to cl1---" );
   //insert 1000 data
   insertData( cl, 1000 );

   println( "---rename cs---" )
   db.renameCS( oldcsName, newcsName );

   checkRenameCSResult( oldcsName, newcsName, 1 );

   cs = db.getCS( newcsName );

   println( "---drop cl1---" )
   //rename cs,drop cl in the end
   cs.dropCL( clName1 );

   println( "---create cl2---" )
   //rename cs,drop cl in the end
   var cl2 = cs.createCL( clName2 );

   println( "---insert 1000 record to cl2---" );
   //insert 1000 data, and check data
   insertData( cl2, 1000 );//review 2：insert没有校验结果

   var recordNum = cl2.count();
   if( recordNum != 1000 )
   {
      throw buildException( "insertData()", "", "check insert record num", "exp record num: 1000", "act num: " + recordNum );
   }

   checkRenameCSResult( oldcsName, newcsName, 1 );

   commDropCS( db, newcsName, true, false, "clean cs---" );
   println( "---end the test---" );
}
