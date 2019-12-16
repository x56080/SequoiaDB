/************************************
*@Description: decimal data use +$
*@author:      zhaoyu
*@createdate:  2016.4.25
**************************************/
function main ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   //insert data
   var doc = [{ a: [{ $decimal: "1" }, { $decimal: "2" }, { $decimal: "3" }, { $decimal: "4" }, { $decimal: "5" }] },
   { a: [{ $decimal: "1" }, { $decimal: "4" }, { $decimal: "5", $precision: [5, 2] }] },
   { a: [{ $decimal: "4" }, { $decimal: "2" }, { $decimal: "1" }] }];
   insertData( dbcl, doc );

   //check result
   var expRecs = [{ a: [{ $decimal: "1" }, { $decimal: "2" }, { $decimal: "3" }, { $decimal: "4" }, { $decimal: "5" }] },
   { a: [{ $decimal: "1" }, { $decimal: "4" }, { $decimal: "5.00", $precision: [5, 2] }] }];
   checkResult( dbcl, { "a.$1": 5 }, null, expRecs, { _id: 1 } );
   checkResult( dbcl, { "a.$1": { $decimal: "5" } }, null, expRecs, { _id: 1 } );
}

main();