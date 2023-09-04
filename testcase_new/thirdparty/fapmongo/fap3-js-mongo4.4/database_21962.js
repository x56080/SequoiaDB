/******************************************** 
@description : database operation
@testcase    : seqDB-21962
@author      : XiaoNi Huang 2020-02-24
*********************************************/
main();

function main ()
{
   var clName = "cl21962";
   var cl = db.getCollection( clName );
   cl.drop();

   // show dbs  ---can't auto, no auto for now

   // drop database
   cl.insert( { "a": 1 } );
   var rc = db.dropDatabase();
   assert.eq( rc, { "dropped": db.toString(), "ok": 1 } );

   // repeat drop database
   var rc = db.dropDatabase();
   assert.eq( rc, { "ok": 1 } );
}