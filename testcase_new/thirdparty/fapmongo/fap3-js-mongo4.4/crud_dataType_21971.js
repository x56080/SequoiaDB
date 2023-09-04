/******************************************** 
@description : crud all dataType
@testcase    : seqDB-21971
@author      : XiaoNi Huang 2020-02-24
*********************************************/
main();

function main ()
{
   var clName = "cl21791";
   var cl = db.getCollection( clName );
   cl.drop();

   // insert
   var date = ISODate();
   var docs = [
      { "_id": 1, "a": "int", "b": -2147483648, "c": 2147483647 },
      { "_id": 2, "a": "long", "b": -9223372036854775808, "c": 9223372036854775807 },
      { "_id": 3, "a": "float", "b": -1.7E+308, "c": 1.7E+308 },
      { "_id": 4, "a": "double", "b": -0.1, "c": 0.0 },
      { "_id": 5, "a": "string", "b": "", "c": "  " },
      { "_id": 6, "a": "objId", "b": ObjectId( "5e5937cca0c92b3882dea802" ) },
      { "_id": 7, "a": "obj", "b": { "b1": {} }, "c": { "c1": { "c2": 1 } } },
      { "_id": 8, "a": "array", "b": [], "c": [1, { "c1": [{}] }] },
      { "_id": 9, "a": "bool", "b": true, "c": false },
      { "_id": 10, "a": "date", "b": date },
      { "_id": 11, "a": "binary", "b": BinData( 3, "aGVsbG8gd29ybGQ=" ) },
      { "_id": 12, "a": "null", "b": "" },
      { "_id": 13, "a": "null", "b": null }
   ];
   cl.insert( docs );


   // find 
   var actRCDocs = [];
   for( var i = 0; i < docs.length; i++ )
   {
      assert.eq( cl.count( docs[i] ), 1 );

      var rc = cl.find( docs[i] ).sort( { "_id": 1 } );
      var rcDoc = rc.next();
      actRCDocs.push( rcDoc );
   }
   assert.eq( JSON.stringify( actRCDocs ), JSON.stringify( docs ) );


   // update
   var updateDocs = [
      { "a": "int", "b": 2147483647, "c": -2147483648 },
      { "a": "long", "b": 9223372036854776000, "c": -9223372036854776000 },
      { "a": "float", "b": 1.7E+308, "c": -1.7E+308 },
      { "a": "double", "b": 0.1, "c": -0.0 },
      { "a": "string", "b": "  ", "c": "" },
      { "a": "objId", "b": ObjectId( "5ebac88f73a50226f0958ee8" ) },
      { "a": "obj", "b": { "b1": { "b2": {} } }, "c": {} },
      { "a": "array", "b": [1, { "b1": [{}] }], "c": [] },
      { "a": "bool", "b": false, "c": true },
      { "a": "date", "b": date },
      { "a": "binary", "b": BinData( 5, "aGVsbG8gd29ybGQ=" ) },
      { "a": "null1", "b": null },
      { "a": "null2", "b": "" }
   ];

   var expDocs = [
      { "_id": 1, "a": "int", "b": 2147483647, "c": -2147483648 },
      { "_id": 2, "a": "long", "b": 9223372036854776000, "c": -9223372036854776000 },
      { "_id": 3, "a": "float", "b": 1.7E+308, "c": -1.7E+308 },
      { "_id": 4, "a": "double", "b": 0.1, "c": -0.0 },
      { "_id": 5, "a": "string", "b": "  ", "c": "" },
      { "_id": 6, "a": "objId", "b": ObjectId( "5ebac88f73a50226f0958ee8" ) },
      { "_id": 7, "a": "obj", "b": { "b1": { "b2": {} } }, "c": {} },
      { "_id": 8, "a": "array", "b": [1, { "b1": [{}] }], "c": [] },
      { "_id": 9, "a": "bool", "b": false, "c": true },
      { "_id": 10, "a": "date", "b": date },
      { "_id": 11, "a": "binary", "b": BinData( 5, "aGVsbG8gd29ybGQ=" ) },
      { "_id": 12, "a": "null1", "b": null },
      { "_id": 13, "a": "null2", "b": "" }
   ];

   var actRCDocs = [];
   for( var i = 0; i < docs.length; i++ )
   {
      var rc = cl.update( { "_id": i + 1 }, updateDocs[i] );
      assert.eq( rc, { "nMatched": 1, "nUpserted": 0, "nModified": 1 } );

      assert.eq( cl.count( updateDocs[i] ), 1 );

      var rc = cl.find( updateDocs[i] ).sort( { "_id": 1 } );
      var rcDoc = rc.next();
      actRCDocs.push( rcDoc );
   }
   assert.eq( JSON.stringify( actRCDocs ), JSON.stringify( expDocs ) );


   // remove   
   for( var i = 0; i < updateDocs.length; i++ )
   {
      var rc = cl.remove( updateDocs[i] );
      assert.eq( rc, { "nRemoved": 1 } );
   }
   assert.eq( cl.count(), 0 );


   cl.drop();
}