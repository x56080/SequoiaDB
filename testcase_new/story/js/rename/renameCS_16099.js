/************************************
*@Description: 修改cs名后，执行数据增删改查操作
*@author:      luweikang
*@createdate:  2018.10.12
*@testlinkCase:seqDB-16099
**************************************/

main();

function main ()
{
   var oldcsName = CHANGEDPREFIX + "_16099_oldcs";
   var newcsName = CHANGEDPREFIX + "_16099_newcs";
   var clName = CHANGEDPREFIX + "_16099_cl";

   var cs = commCreateCS( db, oldcsName, false, "create cs in begine", "" );
   var cl = commCreateCL( db, oldcsName, clName, {}, false, false, "create CL in the begin" );

   println( "---insert 1000 record---" );
   //insert 1000 data
   insertData( cl, 1000, 1000 );

   println( "---rename cs---" );
   db.renameCS( oldcsName, newcsName );

   checkRenameCSResult( oldcsName, newcsName, 1 );

   cl = db.getCS( newcsName ).getCL( clName );

   println( "---insert 1000 record---" );
   //insert 1000 data, and check data
   insertData( cl, 1000, 2000 );

   println( "---update record---" );
   //update ($set: {a:10086}) 2000 data, and check data
   updateData( cl );

   println( "---delete 1000 record---" );
   //delete no < 500 data, and check data
   deleteData( cl );

   commDropCS( db, newcsName, true, false, "clean cs---" );
}

function insertData ( cl, insertNum, expNum )
{
   try
   {

      println( "---begin to insert data " );
      var docs = [];
      for( var i = 0; i < insertNum; ++i )
      {
         var no = i;
         var a = i;
         var user = "test" + i;
         var phone = 13700000000 + i;
         var time = new Date().getTime();
         var doc = { no: no, a: a, customerName: user, phone: phone, openDate: time };
         //data example: {"no":5, customerName:"test5", "phone":13700000005, "openDate":1402990912105

         docs.push( doc );
      }
      cl.insert( docs );

      var recordNum = cl.count();
      if( recordNum != expNum )
      {
         throw buildException( "insertData()", "", "check insert record num", "exp record num: " + expNum, "act num: " + recordNum );
      }
   }
   catch( e )
   {
      throw buildException( "insertData()", e, "insert", "insert success", "insert fail" );
   }
}

function updateData ( cl )
{
   cl.update( { $set: { a: 10086 } } );
   var recordNum = cl.count( { a: 10086 } );
   if( recordNum != 2000 )
   {
      throw buildException( "updateData()", "", "update", "update 2000 record", "update fail, only: " + recordNum );
   }
}

function deleteData ( cl )
{
   cl.remove( { no: { $lt: 500 } } );
   var recordNum = cl.count();
   if( recordNum != 1000 )
   {
      throw buildException( "deleteData()", "", "delete", "delete 1000 record", "delete fail, have: " + recordNum );
   }
}




