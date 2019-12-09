/************************************
*@Description: argument actual precision and max precision check for {$decimal:"xxx",$precision:[xx,xx]}
*@author:      zhaoyu
*@createdate:  2016.5.4
**************************************/
function main ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl 
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0 );

   //insert data
   var doc = [{ a: { $decimal: "123", $precision: [5, 2] } },
   { a: { $decimal: "456", $precision: [3, 0] } }];
   insertData( dbcl, doc );

   //valid argument check
   var expRecs = [{ a: { $decimal: "123.00", $precision: [5, 2] } },
   { a: { $decimal: "456", $precision: [3, 0] } }];
   checkResult( dbcl, {}, {}, expRecs, { _id: 1 } );

   //invalid argument check
   var invalidDoc1 = { a: { $decimal: "789", $precision: [5, 3] } };
   invalidDataInsertCheckResult( dbcl, invalidDoc1, -6 );
}

main();