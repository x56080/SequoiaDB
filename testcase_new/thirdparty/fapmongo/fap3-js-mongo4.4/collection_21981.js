/******************************************** 
@description : collection operation
@testcase    : seqDB-21981
@author      : XiaoNi Huang 2020-02-24
*********************************************/
main();

function main ()
{
   var clName = "cl21981";
   db.dropDatabase();

   // show collections / show tables  ---can't auto, no auto for now

   // getCollectionNames, rc empty
   var rc = db.getCollectionNames();
   assert.eq( rc, [] );

   // create collection
   var rc = db.createCollection( clName + "_1" );
   assert.eq( rc, { "ok": 1 } );
   // repeat create collection
   try
   {
      db.createCollection( clName + "_1" );
   }
   catch( e )
   {
      assert.eq( e, 'Error: [{ "ok" : 0, "code" : -22, "errmsg" : "Collection already exists" }]' );
   }

   // drop and create same collection  
   var cl = db.getCollection( clName + "_1" );
   cl.drop();
   var rc = db.createCollection( clName + "_1" );
   assert.eq( rc, { "ok": 1 } );

   // options not support, create success but not source
   var rc = db.createCollection( clName + "_2", { "autoIndexId": true } );
   assert.eq( rc, { "ok": 1 } );

   // getCollectionNames
   var rc = db.getCollectionNames();
   assert.eq( rc, ["cl21981_1", "cl21981_2"] );

   /* sdb not support, only rc name
   // getCollectionInfos
   var rc = db.getCollectionInfos();
   assert.eq(rc, [ { "name" : "cl21981_1" }, { "name" : "cl21981_2" } ] );
   */

   // drop cl
   var cl = db.getCollection( clName + "_1" );
   var rc = cl.drop();
   assert.eq( rc, true );

   var rc = db.getCollectionNames();
   assert.eq( rc, ["cl21981_2"] );

   // repeat drop cl
   var rc = cl.drop();
   assert.eq( rc, false );


   db.dropDatabase();
}