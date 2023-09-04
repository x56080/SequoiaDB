/******************************************** 
@description : test machers
@testcase    : seqDB-21972
@author      : XiaoNi Huang 2020-02-24
*********************************************/
main();

function main ()
{
   var clName = "cl21972";
   var cl = db.getCollection( clName );
   cl.drop();

   // ready data
   var docsNum = 100;
   var docs = insertDocs( cl, docsNum );

   // $eq
   var rc = cl.find( { "a": { "$eq": 0 } } );
   checkResults( rc, docs.slice( 0, 1 ) );
   // $et
   var rc = cl.find( { "a": { "$et": 0 } } );
   checkResults( rc, docs.slice( 0, 1 ) );

   // $ne
   var rc = cl.find( { "a": { "$ne": 0 } } );
   checkResults( rc, docs.slice( 1, docsNum ) );

   // $and / $lt / $gt
   var rc = cl.find( { "$and": [{ "a": { "$gt": 10 } }, { "a": { "$lt": 20 } }] } );
   checkResults( rc, docs.slice( 11, 20 ) );

   // $or / $lte / $gte
   var rc = cl.find( { "$or": [{ "a": { "$lte": 10 } }, { "a": { "$gte": 90 } }] } );
   checkResults( rc, docs.slice( 0, 11 ).concat( docs.slice( 90 ) ) );

   /* sdb -6, not support
   // $nor 
   //var rc = cl.find( { "$nor": [ { "a": { "$lte": 10 } }, { "a": { "$gte": 90 } } ] } );
   //checkResults ( rc, tmpDocs );
   
   // $not
   //var rc = cl.find( { "a": { "$not": { "$lt": 10 } } } );
   //checkResults ( rc, docs.slice( 10, docsNum ) );
   
   // $not  sdb grammar
   //var rc = cl.find( { "$not": [{ "a":  { "$lt": 10 }  } ] } );
   //checkResults ( rc, docs.slice( 10, docsNum ) );
   */

   // $exists
   var doc = { "_id": docsNum, "c": docsNum };
   cl.insert( doc );
   var rc = cl.find( { "c": { "$exists": 1 } } );
   checkResults( rc, [doc] );
   cl.remove( doc );

   /* sdb -6, not support
   // $type 
   var doc = { "_id": docsNum, "c": "string" };
   cl.insert( doc );
   var rc = cl.find( { "c": { "$type": 2 } } );
   checkResults ( rc, [ doc ] );
   cl.remove( doc );
   
   // $type  sdb grammar
   var doc = { "_id": docsNum, "c": "string" };
   cl.insert( doc );
   var rc = cl.find( { "c": { "$type": 1, "$et": 2 } } );
   checkResults ( rc, [ doc ] );
   cl.remove( doc );
   */

   // $mod
   var rc = cl.find( { "$and": [{ "a": { "$lt": 5 } }, { "a": { "$mod": [3, 1] } }] } );
   checkResults( rc, [docs[1], docs[4]] );

   // $regex
   var tmpDocs = [{ "_id": docsNum, "c": "abc" }, { "_id": docsNum + 1, "c": "test" }];
   cl.insert( tmpDocs );
   var rc = cl.find( { "c": { "$regex": "^a", "$options": "i" } } );
   checkResults( rc, [tmpDocs[0]] );
   cl.remove( { "c": { "$exists": 1 } } );

   // array
   // $all
   var tmpDocs = [{ "_id": docsNum, "c": [1, 2] }, { "_id": docsNum + 1, "c": [1, 2, 3] }];
   cl.insert( tmpDocs );
   var rc = cl.find( { "c": { "$all": [2, 3] } } );
   checkResults( rc, [tmpDocs[1]] );

   /* sdb -6, not support
   // $size
   var rc = cl.find( { "c": { "$size": 3 } } );
   checkResults ( rc, [ tmpDocs[1] ] );  
   cl.remove( { "c": { "$exists": 1 } } );
   
   // $size  sdb grammar
   var rc = cl.find( { "c": { "$size": 1, "$et": 3 } } );
   checkResults ( rc, [ tmpDocs[1] ] );  
   
   cl.remove( { "c": { "$exists": 1 } } );
   */


   cl.drop();
}

function insertDocs ( cl, docsNum )
{
   var docs = new Array();
   for( i = 0; i < docsNum; i++ )
   {
      var doc = { "_id": i, "a": i };
      docs.push( doc );
   }
   cl.insert( docs );
   return docs;
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