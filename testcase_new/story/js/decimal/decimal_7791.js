/************************************
*@Description: argument max precision check for {$decimal:"xxx",$precision:[xx,xx]}
*@author:      zhaoyu
*@createdate:  2016.5.4
**************************************/
function main ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   //insert data
   var doc = [{ a: { $decimal: "9223372036854775807198410", $precision: [1000, 2] } },
   { a: { $decimal: "2", $precision: [1, 0] } }];
   insertData( dbcl, doc );

   //valid argument check
   var expRecs = [{ a: { $decimal: "9223372036854775807198410.00", $precision: [1000, 2] } },
   { a: { $decimal: "2", $precision: [1, 0] } }];

   checkResult( dbcl, {}, {}, expRecs, { _id: 1 } );

   //invalid argument check
   var invalidDoc1 = { a: { $decimal: "2", $precision: [0, 0] } };
   invalidDataInsertCheckResult( dbcl, invalidDoc1, -6 );

   var invalidDoc2 = { a: { $decimal: "9223372036854775807198410", $precision: [1001, 2] } };
   invalidDataInsertCheckResult( dbcl, invalidDoc2, -6 );

   var invalidDoc3 = { a: { $decimal: "2", $precision: ["a", 2] } };
   invalidDataInsertCheckResult( dbcl, invalidDoc3, -6 );

   var invalidDoc4 = { a: { $decimal: "2", $precision: [-1, 2] } };
   invalidDataInsertCheckResult( dbcl, invalidDoc4, -6 );

   var invalidDoc5 = { a: { $decimal: "2", $precision: [3.2, 2] } };
   invalidDataInsertCheckResult( dbcl, invalidDoc5, -6 );

}

main();