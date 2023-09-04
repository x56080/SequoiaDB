/******************************************** 
@description : cursor, cursor.count/limit/skip/sort/batchSize...
@testcase    : seqDB-22392
@author      : XiaoNi Huang 2020-07-02
*********************************************/
main();

function main ()
{
   var clName = "cl22392";
   var cl = db.getCollection( clName );
   cl.drop();

   // ready data
   var docs = [
      { "_id": 1, "a": 5, "b": 2 },
      { "_id": 2, "a": 4, "b": 1 },
      { "_id": 3, "a": 3, "b": 2 },
      { "_id": 4, "a": 2, "b": 1 },
      { "_id": 5, "a": 1, "b": 2 }];
   cl.insert( docs );

   // cursor.size / cursor.count
   // rc size > 0
   var rc = cl.find( { "a": { "$gt": 1 } } ).size();
   var crSize = 4;
   assert.eq( rc, crSize );

   var rc = cl.find( { "a": { "$gt": 1 } } ).count();
   assert.eq( rc, crSize );

   // rc size = 0
   var rc = cl.find( { "a": { "$gt": 1 } } ).size();
   var crSize = 0;

   var rc = cl.find( { "a": { "$lt": 1 } } ).count();
   assert.eq( rc, crSize );

   // cursor.limit
   // limit > cursor.size
   var rc = cl.find().limit( docs.length + 1 );
   checkResults( rc, JSON.stringify( docs ) );
   // limit < cursor.size
   var rc = cl.find().limit( docs.length - 1 );
   checkResults( rc, JSON.stringify( docs.slice( 0, docs.length - 1 ) ) );
   // limit = 0
   var rc = cl.find().limit( 0 );
   checkResults( rc, JSON.stringify( docs ) );
   // limit = -2
   var rc = cl.find().limit( -2 );
   checkResults( rc, JSON.stringify( docs.slice( 0, 2 ) ) );
   // limit empty
   var rc = cl.find().limit();
   checkResults( rc, JSON.stringify( docs ) );
   // limit invalid, sdb rc all docs, mongodb rc code: 9 --not bug
   var rc = cl.find().limit( 'test' );
   checkResults( rc, JSON.stringify( docs ) );


   // cursor.skip
   // skip > cursor.size
   var rc = cl.find().skip( docs.length + 1 );
   checkResults( rc, "[]" );
   // skip < cursor.size
   var rc = cl.find().skip( 1 );
   checkResults( rc, JSON.stringify( docs.slice( 1, docs.length ) ) );
   // skip = 0
   var rc = cl.find().skip( 0 );
   checkResults( rc, JSON.stringify( docs ) );
   // skip = -2, sdb rc all docs, mongodb rc code: 2 --not bug
   var rc = cl.find().skip( -2 );
   checkResults( rc, JSON.stringify( docs ) );
   // skip empty
   var rc = cl.find().skip();
   checkResults( rc, JSON.stringify( docs ) );
   // skip invalid, sdb rc all docs, mongodb rc code: 9 --not bug
   var rc = cl.find().skip( 'test' );
   checkResults( rc, JSON.stringify( docs ) );


   // cursor.sort, base test in composite scene
   // field not exist
   var rc = cl.find().sort( { "notExist": 1 } );
   checkResults( rc, JSON.stringify( docs ) );
   // sort invalid, sdb rc all docs, mongodb rc code: 9 --not bug
   var rc = cl.find().sort( 'test' );
   checkResults( rc, JSON.stringify( docs ) );


   // cursor.batchSize
   // batchSize > cursor.size
   var rc = cl.find().batchSize( docs.length + 1 );
   checkResults( rc, JSON.stringify( docs ) );
   // batchSize < cursor.size
   var rc = cl.find().batchSize( docs.length - 1 );
   checkResults( rc, JSON.stringify( docs ) );
   // batchSize = 0
   var rc = cl.find().batchSize( 0 );
   checkResults( rc, JSON.stringify( docs ) );
   // batchSize = -2   ----SEQUOIADBMAINSTREAM-5977
   //var rc = cl.find().batchSize( -2 );
   //checkResults( rc, JSON.stringify( docs.slice( 0, 2 ) ) ); 
   // batchSize empty
   var rc = cl.find().batchSize();
   checkResults( rc, JSON.stringify( docs ) );
   // batchSize invalid, sdb rc all docs, mongodb rc code: 9 --not bug
   var rc = cl.find().batchSize( 'test' );
   checkResults( rc, JSON.stringify( docs ) );


   // composite scene      
   // find.skip.limit
   var rc = cl.find().skip( 1 ).limit( 2 );
   checkResults( rc, "[{\"_id\":2,\"a\":4,\"b\":1},{\"_id\":3,\"a\":3,\"b\":2}]" );

   // find.skip.limit.sort, sigle field order by
   var rc = cl.find().skip( 1 ).limit( 2 ).sort( { "a": 1 } );
   checkResults( rc, "[{\"_id\":4,\"a\":2,\"b\":1},{\"_id\":3,\"a\":3,\"b\":2}]" );

   // find.skip.limit.sort, sigle field desc order by
   var rc = cl.find().skip( 1 ).limit( 2 ).sort( { "a": -1 } );
   checkResults( rc, "[{\"_id\":2,\"a\":4,\"b\":1},{\"_id\":3,\"a\":3,\"b\":2}]" );

   // find.skip.limit.sort, multi field mix order by
   var rc = cl.find().skip( 1 ).limit( 3 ).sort( { "b": 1, "a": -1 } );
   checkResults( rc, "[{\"_id\":4,\"a\":2,\"b\":1},{\"_id\":1,\"a\":5,\"b\":2},{\"_id\":3,\"a\":3,\"b\":2}]" );

   var rc = cl.find().skip( 1 ).limit( 3 ).sort( { "b": -1, "a": 1 } );
   checkResults( rc, "[{\"_id\":3,\"a\":3,\"b\":2},{\"_id\":1,\"a\":5,\"b\":2},{\"_id\":4,\"a\":2,\"b\":1}]" );


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