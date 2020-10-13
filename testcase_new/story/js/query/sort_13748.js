/******************************************************************************
@Description : 1. Query binary type sort without index
               2. Query binary type forced sort through index
@Modify list :
               2015-01-16 pusheng Ding  Init
               2020-08-14 Zixian Yan    Modify
******************************************************************************/
testConf.clName = COMMCLNAME + "_13748";
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   var indexName = "index_13748";
   var data = [ { a: { "$binary": "Ym9vaw==" }, b: 3, c: "abcd" },
	             { a: { "$binary": "YWdyZWU=" }, b: 2, c: "efghi" },
	             { a: { "$binary": "ZG9n" }, b: 4, c: "xyz" },
	             { a: { "$binary": "Y2F0" }, b: 1, c: "jklmn" } ];

   var expectation = [ { a: { "$binary": "Y2F0" }, b: 1, c: "jklmn" },
                       { a: { "$binary": "YWdyZWU=" }, b: 2, c: "efghi" },
                       { a: { "$binary": "Ym9vaw==" }, b: 3, c: "abcd" },
                       { a: { "$binary": "ZG9n" }, b: 4, c: "xyz" } ];

   cl.insert( data );

   //Query without index
   var query1 = cl.find().sort( {a: 1} );
   checkRec( query1, expectation);

   cl.createIndex( indexName, {a: 1} );

   // Query forced through index
   var query2 = cl.find().hint( { "": indexName } );
   checkRec( query2, expectation );
}
