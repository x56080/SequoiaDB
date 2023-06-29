/***************************************** 
@description : aggregate operation
@testcase    : seqDB-21975
@author      : XiaoNi Huang 2020-02-24
******************************************/
main();

function main ()
{
   var clName = "cl21975";
   var cl = db.getCollection( clName );
   cl.drop();

   // ready data
   var docs = [{ "_id": 1, "a": "A", "b": 86, "c": "test", "d": 1 },
   { "_id": 2, "a": "B", "b": 90, "c": "dev", "d": 1 },
   { "_id": 3, "a": "C", "b": 100, "c": "test", "d": 1 },
   { "_id": 4, "a": "D", "b": 79, "c": "dev", "d": 1 }];
   cl.insert( docs );

   // $progect
   // only one normal field
   // a:1
   var rc = cl.aggregate( { "$project": { "a": 1 } } );
   checkResults( rc, "[{\"_id\":1,\"a\":\"A\"},{\"_id\":2,\"a\":\"B\"},{\"_id\":3,\"a\":\"C\"},{\"_id\":4,\"a\":\"D\"}]" );
   // a:0
   try
   {
      cl.aggregate( { "$project": { "a": 0 } } );
      throw new Error( "expect fail but actual success." );
   }
   catch( e )
   {
      assert.eq( e, 'Error: command failed: { "ok" : 0, "errmsg" : "Exclusion fields is not supported", "code" : -32 } : aggregate failed' );
   }

   // _id:1
   var rc = cl.aggregate( { "$project": { "_id": 1 } } );
   checkResults( rc, "[{\"_id\":1},{\"_id\":2},{\"_id\":3},{\"_id\":4}]" );
   // _id:0
   try
   {
      cl.aggregate( { "$project": { "_id": 0 } } );
      throw new Error( "expect fail but actual success." );
   }
   catch( e )
   {
      assert.eq( e, 'Error: command failed: { "ok" : 0, "errmsg" : "Exclusion fields is not supported", "code" : -32 } : aggregate failed' );
   }

   // multi field   
   // a:1,b:1
   var rc = cl.aggregate( { "$project": { "a": 1, "b": 1 } } );
   checkResults( rc, "[{\"_id\":1,\"a\":\"A\",\"b\":86},{\"_id\":2,\"a\":\"B\",\"b\":90},{\"_id\":3,\"a\":\"C\",\"b\":100},{\"_id\":4,\"a\":\"D\",\"b\":79}]" );
   // a:1,b:0
   var rc = cl.aggregate( { "$project": { "a": 1, "b": 0 } } );
   checkResults( rc, "[{\"_id\":1,\"a\":\"A\"},{\"_id\":2,\"a\":\"B\"},{\"_id\":3,\"a\":\"C\"},{\"_id\":4,\"a\":\"D\"}]" );

   // _id:1,b:1
   var rc = cl.aggregate( { "$project": { "_id": 1, "b": 1 } } );
   checkResults( rc, "[{\"_id\":1,\"b\":86},{\"_id\":2,\"b\":90},{\"_id\":3,\"b\":100},{\"_id\":4,\"b\":79}]" );
   // _id:0,b:1
   var rc = cl.aggregate( { "$project": { "_id": 0, "b": 1 } } );
   checkResults( rc, "[{\"b\":86},{\"b\":90},{\"b\":100},{\"b\":79}]" );
   // _id:0,b:1,c:0
   var rc = cl.aggregate( { "$project": { "_id": 0, "b": 1, "c": 0 } } );
   checkResults( rc, "[{\"b\":86},{\"b\":90},{\"b\":100},{\"b\":79}]" );
   // _id:0,b:1,c:1
   var rc = cl.aggregate( { "$project": { "_id": 0, "b": 1, "c": 1 } } );
   checkResults( rc, "[{\"b\":86,\"c\":\"test\"},{\"b\":90,\"c\":\"dev\"},{\"b\":100,\"c\":\"test\"},{\"b\":79,\"c\":\"dev\"}]" );
   // _id:0, a:0
   try
   {
      cl.aggregate( { "$project": { "_id": 0, "a": 0 } } );
      throw new Error( "expect fail but actual success." );
   }
   catch( e )
   {
      assert.eq( e, 'Error: command failed: { "ok" : 0, "errmsg" : "Exclusion fields is not supported", "code" : -32 } : aggregate failed' );
   }


   // $match + $group + $sort(1) + $limit
   var rc = cl.aggregate( { "$match": { "b": { "$gt": 80 } } }, { "$group": { "_id": "$b", "total": { "$sum": "$b" } } }, { "$sort": { "total": 1 } }, { "$limit": 2 } );
   checkResults( rc, "[{\"_id\":86,\"total\":86},{\"_id\":90,\"total\":90}]" );
   // $match + $group + $sort(-1) + $skip
   var rc = cl.aggregate( { "$match": { "b": { "$gt": 80 } } }, { "$group": { "_id": "$b", "total": { "$sum": "$b" } } }, { "$sort": { "total": -1 } }, { "$skip": 1 } );
   checkResults( rc, "[{\"_id\":90,\"total\":90},{\"_id\":86,\"total\":86}]" );


   // $limit
   // 0
   var rc = cl.aggregate( { "$limit": 0 } );
   checkResults( rc, "[]" );
   // = docs.length
   var rc = cl.aggregate( { "$limit": docs.length }, { "$sort": { "b": 1 } } );
   checkResults( rc, "[{\"_id\":4,\"a\":\"D\",\"b\":79,\"c\":\"dev\",\"d\":1},{\"_id\":1,\"a\":\"A\",\"b\":86,\"c\":\"test\",\"d\":1},{\"_id\":2,\"a\":\"B\",\"b\":90,\"c\":\"dev\",\"d\":1},{\"_id\":3,\"a\":\"C\",\"b\":100,\"c\":\"test\",\"d\":1}]" );


   // $skip 
   // 0
   var rc = cl.aggregate( { "$skip": 0 }, { "$sort": { "b": 1 } } );
   checkResults( rc, "[{\"_id\":4,\"a\":\"D\",\"b\":79,\"c\":\"dev\",\"d\":1},{\"_id\":1,\"a\":\"A\",\"b\":86,\"c\":\"test\",\"d\":1},{\"_id\":2,\"a\":\"B\",\"b\":90,\"c\":\"dev\",\"d\":1},{\"_id\":3,\"a\":\"C\",\"b\":100,\"c\":\"test\",\"d\":1}]" );
   // = docs.length
   var rc = cl.aggregate( { "$skip": docs.length } );
   checkResults( rc, "[]" );


   // $sort
   // field not exist
   var rc = cl.aggregate( { "$sort": { "notEixst": -1 } }, { "$limit": 2 } );
   checkResults( rc, "[{\"_id\":1,\"a\":\"A\",\"b\":86,\"c\":\"test\",\"d\":1},{\"_id\":2,\"a\":\"B\",\"b\":90,\"c\":\"dev\",\"d\":1}]" );


   // $group
   // $first / $last
   var rc = cl.aggregate( [{ "$group": { "_id": "$c", "first_b": { "$first": "$b" }, "max_b": { "$max": "$b" }, "last_b": { "$last": "$b" } } }] );
   //SEQUOIADBMAINSTREAM-5656
   //checkResults( rc, "[{\"_id\":\"dev\",\"first_b\":79,\"max_b\":90,\"last_b\":90},{\"_id\":\"test\",\"first_b\":86,\"max_b\":100,\"last_b\":100}]" );

   // $max / $min
   var rc = cl.aggregate( [{ "$group": { "_id": "$c", "max_b": { "$max": "$b" }, "min_b": { "$min": "$b" }, "push_d": { "$push": "$d" } } }, { "$sort": { "_id": 1 } }] );
   checkResults( rc, "[{\"_id\":\"dev\",\"max_b\":90,\"min_b\":79,\"push_d\":[1,1]},{\"_id\":\"test\",\"max_b\":100,\"min_b\":86,\"push_d\":[1,1]}]" );

   // $avg, $addToSet
   var rc = cl.aggregate( [{ "$group": { "_id": "$c", "avg_b": { "$avg": "$b" }, "addtoset_d": { "$addToSet": "$d" } } }, { "$sort": { "_id": 1 } }] );
   checkResults( rc, "[{\"_id\":\"dev\",\"avg_b\":84.5,\"addtoset_d\":[1]},{\"_id\":\"test\",\"avg_b\":93,\"addtoset_d\":[1]}]" );

   // $sum
   var rc = cl.aggregate( [{ "$group": { "_id": "$c", "sum_b": { "$sum": "$b" } } }, { "$sort": { "_id": 1 } }] );
   checkResults( rc, "[{\"_id\":\"dev\",\"sum_b\":169},{\"_id\":\"test\",\"sum_b\":186}]" );


   // invalid argument
   try
   {
      cl.aggregate( "" );
      throw new Error( "expect fail but actual success." );
   }
   catch( e )
   {
      assert.eq( e, 'Error: command failed: { "ok" : 0, "code" : -6, "codeName" : "Invalid Argument", "errmsg" : "" } : aggregate failed' );
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
   assert.eq( JSON.stringify( docs.sort() ), expDocs );
}