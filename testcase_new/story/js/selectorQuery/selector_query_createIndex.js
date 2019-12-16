/*******************************************************************************
*@Description : When we create index for querying, test query use selector:
*               $include/$default/$slice/$elemMatchOne/$elemMatch
*@Modify list :
*               2015-01-29  xiaojun Hu  Change
*******************************************************************************/

function main ( db )
{
   try
   {
      var recordNum = 100;
      var idxName = "rgIdIndex";
      var idxDef = { "GroupID": -1, "PrimaryNode": 1, "Version": 1 };
      var addRecord1 = { "nest1": { "nest2": { "nest3": "when nest test, use $include" } } };
      var addRecord2 = { "array0": [{ "array1": [{ "array2": ["a", "b", "c", "d", "e", "f", "g", "h", "i"] }] }] };
      var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, {}, true, false,
         "create colleciton in the begnning" );
      // auto generate data and create Index
      commCreateIndex( cl, idxName, idxDef );
      selAutoGenData( cl, recordNum, addRecord1, addRecord2 );
      println( "success to create index: " + idxName +
         " and insert record: " + recordNum );

      /*Test Point 1 $include */
      var condObj = {};
      var selObj = {
         "GroupID": { "$include": 1 },
         "Group.Service.Name": { "$include": 1 },
         "Group.HostName": { "$include": 1 },
         "Version": { "$include": 1 }
      };
      var ret = selMainQuery( cl, condObj, selObj );
      println( "==>success to test use: " + JSON.stringify( selObj ) );
      var condObj = {};
      var selObj = {
         "GroupID": { "$include": 0 },
         "Group.Service.Name": { "$include": 0 },
         "Group.HostName": { "$include": 0 },
         "PrimaryNode": { "$include": 0 }
      };
      var ret = selMainQuery( cl, condObj, selObj );
      println( "==>success to test use: " + JSON.stringify( selObj ) );
      println( "success to query when use $include selector" );

      /*Test Point 2 $default*/
      var condObj = {};
      var selObj = {
         "GroupID": { "$default": [1, 2] },
         "Group.Service.Name": { "$default": [1, 2] },
         "Group.HostName": { "$default": [1, 2] },
         "Version": { "$default": [1, 2] }
      };
      var ret = selMainQuery( cl, condObj, selObj );
      println( "==>success to test use: " + JSON.stringify( selObj ) );

      /*Test Point 3 $slice*/
      var condObj = {};
      var selObj = {
         "Group.Service": { "$slice": [1, 2] },
         "ExtraField2.array0.array1.array2": { "$slice": [-7, 4] }
      };
      var ret = selMainQuery( cl, condObj, selObj );
      println( "==>success to test use: " + JSON.stringify( selObj ) );
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
