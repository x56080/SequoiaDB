/******************************************** 
@description : count
@testcase    : seqDB-21977
@author      : XiaoNi Huang 2020-02-24
*********************************************/
main();

function main ()
{
   var clName = "cl21977";
   var cl = db.getCollection( clName );
   cl.drop();

   // ready data
   var docs = [{ "_id": 1, "a": 1, "b": 1 },
   { "_id": 4, "a": 4, "b": 1 },
   { "_id": 3, "a": 3, "b": 2 },
   { "_id": 2, "a": 2, "b": 2 },
   { "_id": 5, "a": 5, "b": 3 }];
   cl.insert( docs );

   // count
   var rc = cl.count( { "a": { "$gt": 1 } } );
   assert.eq( rc, 4 );

   // find.count
   var rc = cl.find( { "a": { "$gt": 1 } } ).count();
   assert.eq( rc, 4 );

   // cl empty
   cl.remove( {} );
   var rc = cl.count();
   assert.eq( rc, 0 );

   // invalid Argument   
   try
   {
      cl.count( { "$a": 1 } );
      throw new Error( "expect fail but actual success." );
   }
   catch( e )
   {
      var errStr = e.toString().replace( 'Error: count failed: ', '' );
      var errJson = JSON.parse( errStr );
      delete errJson.ErrNodes;
      assert.eq( errJson, { "ok": 0, "code": -6, "codeName": "Invalid Argument", "errmsg": "" } );
   }

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
   assert.eq( JSON.stringify( docs ), JSON.stringify( expDocs ) );
}