/******************************************** 
@description : index
@testcase    : seqDB-21978
@author      : XiaoNi Huang 2020-02-24
*********************************************/
main();

function main ()
{
   var clName = "cl21978";
   var cl = db.getCollection( clName );
   cl.drop();

   // ready data
   var docs = [{ "_id": 1, "a": 1, "b": 1, "c": 1, "d": 1 },
   { "_id": 2, "a": 2, "b": 2, "c": 2, "d": 2 },
   { "_id": 3, "a": 2, "b": 3, "c": 3, "d": 2 }];
   cl.insert( docs );

   // create normal index, not set name
   var rc = cl.createIndex( { "a": 1 } );
   assert.eq( JSON.stringify( rc ), "{\"ok\":1}" );

   // create normal index, set name
   var rc = cl.createIndex( { "b": 1 }, { "name": "bIdx" } );
   assert.eq( JSON.stringify( rc ), "{\"ok\":1}" );

   // create unique index
   var rc = cl.createIndex( { "c": 1 }, { "unique": true } );
   assert.eq( JSON.stringify( rc ), "{\"ok\":1}" );

   // exist unique index, insert duplicate key
   var rc = cl.insert( { "c": 1, "c1": 1 } );
   // SEQUOIADBMAINSTREAM-5513
   //assert.eq( JSON.stringify( rc ), "{\"ok\":0,\"code\":-38,\"errmsg\":\"Duplicate key exist\"}" );
   assert.eq( cl.count( { "c": 1, "c1": 1 } ), 0 );

   // getIndexes
   var rc = cl.getIndexes();
   assert.eq( JSON.stringify( rc.sort() ), ["[{\"v\":0,\"key\":{\"_id\":1},\"name\":\"_id_\",\"ns\":\"" + cl.toString() + "\"},{\"v\":0,\"key\":{\"a\":1},\"name\":\"a_1\",\"ns\":\"" + cl.toString() + "\"},{\"v\":0,\"key\":{\"b\":1},\"name\":\"bIdx\",\"ns\":\"" + cl.toString() + "\"},{\"v\":0,\"key\":{\"c\":1},\"name\":\"c_1\",\"ns\":\"" + cl.toString() + "\"}]"] );


   // drop unique index
   var rc = cl.dropIndex( "c_1" );
   assert.eq( JSON.stringify( rc ), "{\"ok\":1}" );
   // create again
   var rc = cl.createIndex( { "c": 1 }, { "unique": true } );
   assert.eq( JSON.stringify( rc ), "{\"ok\":1}" );

   // drop normal index, by name
   var rc = cl.dropIndex( "bIdx" );
   assert.eq( JSON.stringify( rc ), ["{\"ok\":1}"] );
   var rc = db.getLastError();
   assert.eq( rc, null );

   // drop id index
   var rc = cl.dropIndex( "$id" );
   delete rc.ErrNodes;
   assert.eq( rc, {
      "ok": 0,
      "code": -56,
      "codeName": "$id index can't be dropped",
      "errmsg": "Cannot drop $id index, use dropIdIndex() instead"
   } );
   var rc = db.getLastError();
   assert.eq( rc, "Cannot drop $id index, use dropIdIndex() instead" );

   // getIndexes
   var rc = cl.getIndexes();
   assert.eq( JSON.stringify( rc.sort() ), ["[{\"v\":0,\"key\":{\"_id\":1},\"name\":\"_id_\",\"ns\":\"" + cl.toString() + "\"},{\"v\":0,\"key\":{\"a\":1},\"name\":\"a_1\",\"ns\":\"" + cl.toString() + "\"},{\"v\":0,\"key\":{\"c\":1},\"name\":\"c_1\",\"ns\":\"" + cl.toString() + "\"}]"] );


   // exist duplicate key, create unique index
   cl.remove( {} );
   var rc = cl.createIndex( { "d": 1 }, { "name": "dIdx", "unique": true } );
   assert.eq( rc, { "ok": 1 } );
   // check results
   var rc = cl.insert( { "_id": 1, "d": 1 } );
   assert.eq( rc, { "nInserted": 1 } );
   var rc = cl.insert( { "_id": 2, "d": 1 } );
   // SEQUOIADBMAINSTREAM-5513
   //assert.eq( JSON.stringify( rc ), "{\"ok\":0,\"code\":-38,\"errmsg\":\"Duplicate key exist\"}" );


   cl.drop();
}

function checkResults ( rc, expDocs )
{
   var docs = new Array();
   while( rc.hasNext() )
   {
      var doc = rc.next();
      docs.push( doc );
   }
   assert.eq( JSON.stringify( docs ), expDocs );
}