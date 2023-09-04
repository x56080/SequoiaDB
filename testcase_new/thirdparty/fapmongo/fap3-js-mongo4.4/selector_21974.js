/******************************************** 
@description : test selector
@testcase    : seqDB-21974
@author      : XiaoNi Huang 2020-02-24
*********************************************/
main();

function main ()
{
   var clName = "cl21974";
   var cl = db.getCollection( clName );
   cl.drop();

   // ready data
   var docs = [{ "_id": 1, "a": 1, "b": 1 },
   { "_id": 2, "a": 2, "b": [1, 2, 3, 4, 5] },
   { "_id": 3, "a": 3, "b": [{ "b1": 100 }, { "b1": 101 }] }];
   cl.insert( docs );

   // find by selector
   /*
   // $  ---sdb not support, sdb rc: { "_id" : 2, "b" : [ ] }
   var rc = cl.find( { "b": { "$gt": 2 } }, { "b.$": 1 } );
   checkResults ( rc, ["[{\"_id\":2,\"b\":[3]}]"] );
   */

   // $elemMatch 
   // mongodb rc  : { "_id" : 3, "b" : [ { "b1" : 100 } ] }
   // sequoiadb rc: { "_id" : 3, "a" : 3, "b" : [ { "b1" : 100 } ] }
   // not bug
   // field and value exist, $elemMatch not matcher
   var rc = cl.find( { "a": 3 }, { "b": { "$elemMatch": { "b1": 100 } } } );
   checkResults( rc, "[{\"_id\":3,\"a\":3,\"b\":[{\"b1\":100}]}]" );
   // field not exist
   // mongodb rc  : { "_id" : 3 ] }
   // sequoiadb rc: { "_id" : 3, "a" : 3, "b" : [ { "b1" : 100 }, { "b1" : 101 } ] }
   // not bug
   var rc = cl.find( { "a": 3 }, { "notExist": { "$elemMatch": { "b1": 100 } } } );
   checkResults( rc, "[{\"_id\":3,\"a\":3,\"b\":[{\"b1\":100},{\"b1\":101}]}]" );
   // value not exist
   var rc = cl.find( { "a": 3 }, { "b": { "$elemMatch": { "b1": "notExist" } } } );
   checkResults( rc, "[{\"_id\":3,\"a\":3,\"b\":[]}]" );
   // $elemMatch by matcher
   var rc = cl.find( {}, { "b": { "$elemMatch": { "b1": { "$gt": 100 } } } } );
   checkResults( rc, "[{\"_id\":1,\"a\":1},{\"_id\":2,\"a\":2,\"b\":[]},{\"_id\":3,\"a\":3,\"b\":[{\"b1\":101}]}]" );


   // $slice: int
   var rc = cl.find( { "a": 2 }, { "b": { "$slice": 2 } } );
   checkResults( rc, "[{\"_id\":2,\"a\":2,\"b\":[1,2]}]" );
   // $slice: array
   var rc = cl.find( { "a": 2 }, { "b": { "$slice": [2, 3] } } );
   checkResults( rc, "[{\"_id\":2,\"a\":2,\"b\":[3,4,5]}]" );
   // _id: 0 | 1
   var rc = cl.find( { "a": 2 }, { "_id": 1, "b": { "$slice": [2, 3] } } );
   checkResults( rc, "[{\"_id\":2,\"b\":[3,4,5]}]" );

   // { field: 0 | 1 | true | false  }
   var rc = cl.find( { "a": 1 }, { "b": 0 } )
   checkResults( rc, "[{\"_id\":1,\"a\":1}]" );

   var rc = cl.find( { "a": 1 }, { "b": 1 } )
   checkResults( rc, "[{\"_id\":1,\"b\":1}]" );

   var rc = cl.find( { "a": 1 }, { "b": true } )
   checkResults( rc, "[{\"_id\":1,\"b\":1}]" );

   var rc = cl.find( { "a": 1 }, { "b": false } )
   checkResults( rc, "[{\"_id\":1,\"a\":1}]" );

   var rc = cl.find( { "a": 1 }, { "_id": 0 } )
   checkResults( rc, "[{\"a\":1,\"b\":1}]" );

   var rc = cl.find( { "a": 1 }, { "_id": 1 } )
   checkResults( rc, "[{\"_id\":1}]" );


   // selector include multi field
   // _id:0, a:1
   var rc = cl.find( { "a": 1 }, { "_id": 0, "a": 1 } )
   checkResults( rc, "[{\"a\":1}]" );

   // _id:1, a:1
   var rc = cl.find( { "a": 1 }, { "_id": 1, "a": 1 } )
   checkResults( rc, "[{\"_id\":1,\"a\":1}]" );

   // _id:0, a:0
   var rc = cl.find( { "a": 1 }, { "_id": 0, "a": 0 } )
   checkResults( rc, "[{\"b\":1}]" );

   // _id:0, a:1, b:1
   var rc = cl.find( { "a": 1 }, { "_id": 0, "a": 1, "b": 1 } )
   checkResults( rc, "[{\"a\":1,\"b\":1}]" );

   // _id:0, a:1, b:0
   try
   {
      cl.find( { "a": 1 }, { "_id": 0, "a": 1, "b": 0 } );
   }
   catch( e )
   {
      assert.eq( JSON.stringify( e ), "-6" );
   }

   // field not exist    
   var rc = cl.find( { "a": 1 }, { "notExistFiled": true } )
   checkResults( rc, "[{\"_id\":1}]" );

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