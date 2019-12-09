/*******************************************************************************
*@Description : test selector: $include nest 6 layer array
*@Example: record: {a:{b:{c:["d", "e"}},x:[{y:[{z:["m", "n"]}]}]}/
*          query: db.cs.cl.find({},{"a.b.c":{$include:1}}})
*@Modify list :
*               2015-01-29  xiaojun Hu  Change
*******************************************************************************/

function main ( db )
{
   try
   {
      var recordNum = 100;
      var addRecord1 = { "nest1": { "nest2": { "nest3": ["a", "b"] } } };
      var addRecord2 = [{ "nest1": [{ "nest2": [{ "nest3": ["a", "b"] }] }] }];
      var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0, true, true, false,
         "create colleciton in the begnning" );
      // auto generate data
      selAutoGenData( cl, recordNum, addRecord1, addRecord2 );
      println( "success to insert record: " + recordNum );

      /*Test Point 1 $include=1, different field name do $include querying*/
      var condObj = {};
      var selObj = {
         "ExtraField1.nest1.nest2.nest3": { "$include": 1 },
         "Group.HostName": { "$include": 1 },
         "Group.dbpath": { "$include": 1 },
         "Group.NodeID": { "$include": 1 },
         "Group.Service.Name": { "$include": 1 },
         "PrimaryNode": { "$include": 1 },
         "ExtraField2.nest1.nest2.nest3": { "$include": 1 }
      };
      var ret = selMainQuery( cl, condObj, selObj );
      selVerifyIncludeRet( ret, selObj, 1, "1:NONUM:NONUM:NONUM:4:1:1" );
      //println( "query selector: " + JSON.stringify( selObj ) ) ;
      /*Test Point 1 $include=1, different field name do $include querying*/
      var condObj = {};
      var selObj = {
         "ExtraField1.nest1.nest2.nest3": { "$include": 0 },
         "Group.HostName": { "$include": 0 },
         "Group.dbpath": { "$include": 0 },
         "Group.NodeID": { "$include": 0 },
         "Group.Service.Name": { "$include": 0 },
         "PrimaryNode": { "$include": 0 },
         "ExtraField2.nest1.nest2.nest3": { "$include": 0 }
      };
      var ret = selMainQuery( cl, condObj, selObj );
      selVerifyIncludeRet( ret, selObj, 1, "0:0:0:0:0:0:0" );
      //println( "query selector: " + JSON.stringify( selObj ) ) ;
   }
   catch( e )
   {
      throw e;
   }
}


// Run Main
try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
      "drop collection in the begining" );
   main( db );
   //commDropCL( db, COMMCSNAME, COMMCLNAME, false, false,
   //            "drop collection in the end") ;
}
catch( e )
{
   //commDropCL( db, COMMCSNAME, COMMCLNAME, false, false,
   //            "drop collection in the end") ;
   throw e;
}
