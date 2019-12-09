/************************************
*@Description: decimal data use $feild
*@author:      zhaoyu
*@createdate:  2016.4.25
**************************************/
function main ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0 );

   //insert data
   var doc = [{ t1: { $decimal: "12" }, t2: { $decimal: "12", $precision: [5, 2] } },
   { t1: { $decimal: "12" }, t2: 12 },
   { t1: { $decimal: "12" }, t2: { $decimal: "13", $precision: [5, 2] } }];
   insertData( dbcl, doc );

   //check result
   var expRecs = [{ t1: { $decimal: "12" }, t2: { $decimal: "12.00", $precision: [5, 2] } },
   { t1: { $decimal: "12" }, t2: 12 }];
   checkResult( dbcl, { t1: { $field: "t2" } }, null, expRecs, { _id: 1 } );
}

main();