/*******************************************************************************
*@Description : $include: nest array
*@Example: record: {a:{b:{c:["d", "e"}}}/{a:[{b:[{c:["d", "e"]}]}]}/
*                  {a:[{b:[{c:["d": "e"]}]}]}
*          query: db.cs.cl.find({},{"a.b.c":{$include:1/0}}})
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

      /*【Test Point 1】 record: {a:{b:{c:["d", "e"}}}*/
      var condObj = {};
      var selObj = { "ExtraField1.nest1.nest2.nest3": { "$include": 1 } };
      var ret = selMainQuery( cl, condObj, selObj );
      // verify
      selVerifyIncludeRet( ret, selObj, 1, "1" );
      println( "==>success to test use: " + JSON.stringify( selObj ) );
      var condObj = {};
      var selObj = { "ExtraField1.nest1.nest2.nest3": { "$include": 0 } };
      var ret = selMainQuery( cl, condObj, selObj );
      // verify
      selVerifyIncludeRet( ret, selObj, 0, "0" );
      println( "==>success to test use: " + JSON.stringify( selObj ) );

      /*【Test Point 2】 {a:[{b:[{c:["d", "e"]}]}]}*/
      var condObj = {};
      var selObj = { "Group.Service.Name": { "$include": 1 } };
      var ret = selMainQuery( cl, condObj, selObj );
      // verify
      selVerifyIncludeRet( ret, selObj, 1, "4" );
      println( "==>success to test use: " + JSON.stringify( selObj ) );
      var condObj = {};
      var selObj = { "Group.Service.Name": { "$include": 0 } };
      var ret = selMainQuery( cl, condObj, selObj );
      // verify
      selVerifyIncludeRet( ret, selObj, 0, "0" );
      println( "==>success to test use: " + JSON.stringify( selObj ) );

      /*【Test Point 3】 {a:[{b:[{c:["d": "e"]}]}]}*/
      var condObj = {};
      var selObj = { "ExtraField2.nest1.nest2.nest3": { "$include": 1 } };
      var ret = selMainQuery( cl, condObj, selObj );
      // verify
      selVerifyIncludeRet( ret, selObj, 1, "1" );
      println( "==>success to test use: " + JSON.stringify( selObj ) );
      var condObj = {};
      var selObj = { "ExtraField2.nest1.nest2.nest3": { "$include": 0 } };
      var ret = selMainQuery( cl, condObj, selObj );
      // verify
      selVerifyIncludeRet( ret, selObj, 0, "0" );
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
