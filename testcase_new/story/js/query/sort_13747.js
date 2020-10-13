/******************************************************************************
@Description : 1. Query date type sort without index
               2. Query date type forced sort through index
@Modify list :
               2015-01-16 pusheng Ding  Init
******************************************************************************/
testConf.clName = COMMCLNAME + "_13747";
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   var indexName = "index_13747";
   var data = [ { a: { "$date": "2000-04-03" }, b: 2, c: "abcd" },
             	 { a: { "$date": "2000-04-01" }, b: 1, c: "efghi" },
             	 { a: { "$date": "2011-01-01" }, b: 4, c: "xyz" },
             	 { a: { "$date": "2000-05-01" }, b: 3, c: "jklmn" } ];

   var expectation = [ { a: { "$date": "2000-04-01" }, b: 1, c: "efghi" },
                       { a: { "$date": "2000-04-03" }, b: 2, c: "abcd" },
             	        { a: { "$date": "2000-05-01" }, b: 3, c: "jklmn" },
                       { a: { "$date": "2011-01-01" }, b: 4, c: "xyz" } ];

   cl.insert( data );

   //Query without index
   var query1 = cl.find().sort( {a: 1} );
   checkRec( query1, expectation);

   cl.createIndex( indexName, {a: 1} );

   // Query forced through index
   var query2 = cl.find().hint( { "": indexName } );
   checkRec( query2, expectation)
}
