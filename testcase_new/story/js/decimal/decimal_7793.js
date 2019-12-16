/************************************
*@Description: argument max precision and max scale check for {$decimal:"xxx",$precision:[xx,xx]}
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
   var doc = [{ a: { $decimal: "123", $precision: [5, 2] } }];
   insertData( dbcl, doc );

   //valid argument check
   var expRecs = [{ a: { $decimal: "123.00", $precision: [5, 2] } }];
   checkResult( dbcl, {}, {}, expRecs, { _id: 1 } );

   //invalid argument check
   var invalidDoc1 = { a: { $decimal: "2", $precision: [2, 2] } };
   invalidDataInsertCheckResult( dbcl, invalidDoc1, -6 );

}

main();