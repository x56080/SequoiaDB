/************************************
*@Description: 创建固定集合，使用_id字段执行查询
*@author:      liuxiaoxuan
*@createdate:  2018.4.18
*@testlinkCase:seqDB-15125
**************************************/

var currentLastLogicalID = 0;
var blockCounts = 1;
function main ()
{
   var csName = CHANGEDPREFIX + "_15125_large_CS";
   var clName = CHANGEDPREFIX + "_15125_large_CL";

   //clean and createCS CL before test
   println( "---begin test---" );
   commDropCS( db, csName, true, "drop CS at begin" );

   var csOption = { Capped: true };
   commCreateCS( db, csName, false, "", csOption );

   //create cappedCL
   var options = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: false };
   var dbcl = commCreateCL( db, csName, clName, options, false, false, "create capped cl" )

   //insert 100 records at first extend
   var doc = [];
   var insertNum = 100;
   for( var i = 0; i < insertNum; i++ )
   {
      doc.push( { a: i } );
   }
   insertDatas( dbcl, doc );

   //insert 10w records at middle, each record lengths 968 
   var insertNum = 100000;
   var stringLength = 968;
   insertFixedLengthDatas( dbcl, insertNum, stringLength, 'a' );

   //insert 100 records at the last extend
   var doc = [];
   var insertNum = 100;
   for( var i = 100000; i < 100000 + insertNum; i++ )
   {
      doc.push( { a: i } );
   }
   insertDatas( dbcl, doc );

   //check count
   checkCount( dbcl, {}, 100200 );

   //get all logicalIDs
   var expLogicalIDs = getExpectLogicalIDs( 1, 100 );
   expLogicalIDs = expLogicalIDs.concat( getExpectLogicalIDs( 968, 100000 ) ).concat( getExpectLogicalIDs( 1, 100 ) );

   //get all records
   var allResults = [];
   for( var i = 0; i < 100; i++ )
   {
      allResults.push( { a: i } );
   }
   for( var i = 0; i < 100000; i++ )
   {
      allResults.push( { a: 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa' } );
   }
   for( var i = 100000; i < 100100; i++ )
   {
      allResults.push( { a: i } );
   }

   //$gt、$lt
   var gtObj = { _id: { $gt: 0, $lt: 5600 } };
   checkQueryResult( dbcl, gtObj, null, { _id: 1 }, allResults.slice( 1, 100 ) )
   checkLogicalID( dbcl, gtObj, null, { _id: 1 }, null, null, expLogicalIDs.slice( 1, 100 ) );

   //$gte、$lte
   var gteObj = { _id: { $gte: 0, $lte: 5600 } };
   checkQueryResult( dbcl, gteObj, null, { _id: 1 }, allResults.slice( 0, 101 ) )
   checkLogicalID( dbcl, gteObj, null, { _id: 1 }, null, null, expLogicalIDs.slice( 0, 101 ) );

   //$et
   var etObj = { _id: { $et: 56 } }
   checkQueryResult( dbcl, etObj, null, { _id: 1 }, [{ "a": 1 }] );
   checkLogicalID( dbcl, etObj, null, { _id: 1 }, null, null, expLogicalIDs.slice( 1, 2 ) );

   //$$in
   var neinObj = { "$and": [{ "_id": { "$ne": 0 } }, { "income": { "$lt": 10000 } }] }
   var inObj = { _id: { $in: [0, 56] } }
   checkQueryResult( dbcl, inObj, null, { _id: 1 }, allResults.slice( 0, 2 ) );
   checkLogicalID( dbcl, inObj, null, { _id: 1 }, null, null, expLogicalIDs.slice( 0, 2 ) );

   //$ne、$nin
   var neinObj = { "$and": [{ "_id": { "$ne": 0 } }, { _id: { $nin: [56, 112] } }] };
   checkQueryResult( dbcl, neinObj, null, { _id: 1 }, allResults.slice( 3 ) );
   checkLogicalID( dbcl, neinObj, null, { _id: 1 }, null, null, expLogicalIDs.slice( 3 ) );

   //$mod
   var modObj = { _id: { $mod: [100000000, 0] } }
   checkQueryResult( dbcl, modObj, null, { _id: 1 }, [{ a: 0 }] );
   checkLogicalID( dbcl, modObj, null, { _id: 1 }, null, null, expLogicalIDs.slice( 0, 1 ) );

   //$all
   var allObj = { _id: { $all: [0] } }
   checkQueryResult( dbcl, allObj, null, { _id: 1 }, [{ "a": 0 }] );
   checkLogicalID( dbcl, allObj, null, { _id: 1 }, null, null, expLogicalIDs.slice( 0, 1 ) );

   //$field
   var fieldObj = { '_id': { $field: 'a' } };
   checkQueryResult( dbcl, fieldObj, null, { _id: 1 }, [{ "a": 0 }] );
   checkLogicalID( dbcl, fieldObj, null, { _id: 1 }, null, null, expLogicalIDs.slice( 0, 1 ) );

   println( "---end the test---" );
   commDropCS( db, csName, true, "drop CS in the end" );
}

function checkQueryResult ( dbcl, FindOptions, selectorOptions, sortOptions, results )
{
   try
   {
      var rc = dbcl.find( FindOptions, selectorOptions ).sort( sortOptions );
      checkRec( rc, results );
   }
   catch( e )
   {
      throw buildException( "checkQueryResult()", e, "find record", "find success", "find fail:" + e );
   }
}

function getExpectLogicalIDs ( stringLength, recordNums )
{
   var expLogicalIDs = [];

   var recordLength = stringLength + 55;
   if( recordLength % 4 !== 0 )
   {
      recordLength = recordLength + ( 4 - recordLength % 4 );
   }

   for( var i = 0; i < recordNums; i++ )
   {
      expLogicalIDs.push( currentLastLogicalID );
      var nextLogicalID = currentLastLogicalID + recordLength;

      currentLastLogicalID = nextLogicalID;
      if( currentLastLogicalID > ( blockCounts * 33554396 - recordLength ) )
      { //if the next record length is up to current block size,it will be put to the next block
         currentLastLogicalID = blockCounts * 33554396;
         ++blockCounts;
      }
   }

   return expLogicalIDs;
}

main();