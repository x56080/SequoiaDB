/************************************
*@Description: decimal data use elemMatch
*@author:     zhaoyu
*@createdate:  2016.4.25
**************************************/
function main ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0 );

   //insert data
   var doc = [{ a: { age: { $decimal: "26" }, weight: { $decimal: "50.56", $precision: [10, 2] } } },
   { a: { age: { $decimal: "25" }, weight: { $decimal: "50.56", $precision: [10, 2] } } }];
   insertData( dbcl, doc );

   //check result
   var expRecs = [{ a: { age: { $decimal: "26" }, weight: { $decimal: "50.56", $precision: [10, 2] } } }];
   checkResult( dbcl, { a: { $elemMatch: { age: { $decimal: "26" }, weight: 50.56 } } }, null, expRecs, { _id: 1 } );
   checkResult( dbcl, { a: { $elemMatch: { age: { $decimal: "26", $precision: [10, 2] }, weight: { $decimal: "50.56", $precision: [10, 3] } } } }, null, expRecs, { _id: 1 } );
   checkResult( dbcl, { a: { $elemMatch: { age: 26, weight: 50.56 } } }, null, expRecs, { _id: 1 } );
}

main();	