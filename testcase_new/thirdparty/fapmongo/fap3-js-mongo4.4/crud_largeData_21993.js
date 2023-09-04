/******************************************** 
@description : CRUD large data 
@testcase    : seqDB-22239
@author      : XiaoNi Huang 2020-02-24
*********************************************/
main();

function main ()
{
   var clName = "cl21993";
   var docsNum = 2100;
   var cl = db.getCollection( clName );
   cl.drop();

   // insert
   var docs = [];
   for( var i = 0; i < docsNum; i++ ) 
   {
      docs.push( { "_id": i, "a": i, "b": 1 } );
   }
   var rc = cl.insert( docs );
   assert.eq( JSON.stringify( rc ), ["{\"nInserted\":2100,\"nUpserted\":0,\"nMatched\":0,\"nModified\":0,\"nRemoved\":0}"] );

   // find
   // find / cursor.limit
   // rc all
   var rc = cl.find().sort( { "_id": 1 } );
   checkResults( rc, docs );
   // rc docsNum = 0
   var rc = cl.find( { "notExistField": 1 } );
   checkResults( rc, [] );
   // rc docsNum = 1
   var rc = cl.find( docs[0] );
   checkResults( rc, [docs[0]] );

   var expDocsNum = 999;
   var rc = cl.find().limit( expDocsNum ).sort( { "_id": 1 } );
   checkResults( rc, docs.slice( 0, expDocsNum ) );

   var expDocsNum = 1000;
   var rc = cl.find().limit( expDocsNum ).sort( { "_id": 1 } );
   checkResults( rc, docs.slice( 0, expDocsNum ) );

   var expDocsNum = 1001;
   var rc = cl.find().limit( expDocsNum ).sort( { "_id": 1 } );
   checkResults( rc, docs.slice( 0, expDocsNum ) );

   // cursor.batchSize
   var bs = 0;
   var rc = cl.find().batchSize( bs ).sort( { "_id": 1 } );
   checkResults( rc, docs );

   var bs = 999;
   var rc = cl.find().batchSize( bs ).sort( { "_id": 1 } );
   checkResults( rc, docs );

   var bs = 1000;
   var rc = cl.find().batchSize( bs ).sort( { "_id": 1 } );
   checkResults( rc, docs );

   var bs = 1001;
   var rc = cl.find().batchSize( bs ).sort( { "_id": 1 } );
   checkResults( rc, docs );


   // update
   expDocsNum = 2010
   var expDocs = [];
   for( var i = 0; i < expDocsNum; i++ ) 
   {
      expDocs.push( { "_id": i, "a": i, "b": 2 } );
   }
   var rc = cl.update( { "a": { "$lt": expDocsNum } }, { "$inc": { "b": 1 } }, { "multi": true } );
   assert.eq( rc, { "nMatched": expDocsNum, "nUpserted": 0, "nModified": expDocsNum } );
   // check update
   var rc = cl.find( { "a": { "$lt": expDocsNum } } ).sort( { "_id": 1 } );
   checkResults( rc, expDocs );
   // check not update
   var rc = cl.find( { "a": { "$gte": expDocsNum } } ).sort( { "_id": 1 } );
   checkResults( rc, docs.slice( expDocsNum, docsNum ) );

   // deleteMany
   var deleteDocsNum = 2010;
   var rc = cl.remove( { "a": { "$lt": deleteDocsNum } } );
   assert.eq( rc, { "nRemoved": deleteDocsNum } );

   var rc = cl.find().sort( { "_id": 1 } );
   checkResults( rc, docs.slice( expDocsNum, docsNum ) );


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