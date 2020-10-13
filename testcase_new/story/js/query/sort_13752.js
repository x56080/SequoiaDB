/******************************************************************************
@Description : Precondition - Query sort by index whose field may not included in data
               1. Query sort without index
               2. Query sort forced through index
@Modify list :
               2015-01-17 pusheng Ding  Init
               2020-08-14 Zixian Yan    Modify
******************************************************************************/
testConf.clName = COMMCLNAME + "_13752";
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   var indexName = "index_13752";
   var data = [ { a: 1, b: "string" },
                { b: "max" },
                { a: { "$binary": "aGVsbG8gd29ybGQ=" }, b: "min" },
                { b: "empty" } ];

   var expectation = [ { b: "max" },
                       { b: "empty" },
                       { a: 1, b: "string" },
                       { a: { "$binary": "aGVsbG8gd29ybGQ=" }, b: "min" } ];

   cl.insert( data );

   //Query without index
   var query1 = cl.find().sort( {a: 1} );
   checkRec( query1, expectation);

   cl.createIndex( indexName, {a: 1} );

   // Query forced through index
   var query2 = cl.find().hint( { "": indexName } );
   checkRec( query2, expectation );
}
