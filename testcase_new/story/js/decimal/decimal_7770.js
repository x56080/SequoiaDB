/************************************
*@Description: decimal data use ceiling
*@author:      zhaoyu
*@createdate:  2016.4.26
**************************************/
function main ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0 );

   //insert data
   var doc = [{ a: { $decimal: "1.7E+400", $precision: [1000, 10] } },
   { a: { $decimal: "-5.94E-400", $precision: [1000, 600] } },
   { a: { $decimal: "0" } },
   { a: { $decimal: "-92233720368547758071234567899.123456789" } },
   { a: { $decimal: "92233720368547758071234567899.523456789" } },
   { a: { $decimal: "MAX" } },
   { a: { $decimal: "MIN" } },
   { a: { $decimal: "NAN" } },
   { a: { $decimal: "-INF" } },
   { a: { $decimal: "INF" } }];
   insertData( dbcl, doc );

   //check result           
   var expRecs = [{ a: { $decimal: "17000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" } },
   { a: { $decimal: "0" } },
   { a: { $decimal: "0" } },
   { a: { $decimal: "-92233720368547758071234567899" } },
   { a: { $decimal: "92233720368547758071234567900" } },
   { a: { $decimal: "NaN" } },
   { a: { $decimal: "NaN" } },
   { a: { $decimal: "NaN" } },
   { a: { $decimal: "NaN" } },
   { a: { $decimal: "NaN" } }];
   checkResult( dbcl, {}, { a: { $ceiling: 1 } }, expRecs, { id: -1 } );
}

main();