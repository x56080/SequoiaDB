/*******************************************************************************
*@Description : test selector: $include
*@Example: record: {a:"b"}
*          query: db.cs.cl.find({},{"a":{$include:1/0}}})
*          record: {a:{b:{c:{d:"e"}}}}
*          query: db.cs.cl.find({},{"a.b.c.d":{$include:1/0}}})
*@Modify list :
*               2015-01-29  xiaojun Hu  Change
*******************************************************************************/

function main ( db )
{
   try
   {
      var recordNum = 1;
      var addRecord = { "nest1": { "nest2": { "nest3": "when nest test, use $include" } } };
      var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, {}, true, false,
         "create colleciton in the begnning" );
      // auto generate data
      selAutoGenData( cl, recordNum, addRecord );
      println( "success to insert record: " + recordNum );

      /*【Test Point 1】 $include=1*/
      var condObj = {};
      var selObj = { "GroupID": { "$include": 1 } };
      var ret = selMainQuery( cl, condObj, selObj );
      // verify the result of query
      selVerifyIncludeRet( ret, selObj, 1, "1" );   // common function
      println( "==>success to test use: " + JSON.stringify( selObj ) );

      /*【Test Point 2】 $include=0*/
      var condObj = {};
      var selObj = { "GroupID": { "$include": 0 } };
      var ret = selMainQuery( cl, condObj, selObj );
      // verify the result of query
      selVerifyIncludeRet( ret, selObj, 0, "0" );   // common function
      println( "==>success to test use: " + JSON.stringify( selObj ) );

      /*【Test Point 3】 nest field $include=1*/
      var condObj = {};
      var selObj = { "ExtraField1.nest1.nest2.nest3": { "$include": 1 } };
      var ret = selMainQuery( cl, condObj, selObj );
      // verify
      selVerifyIncludeRet( ret, selObj, 1, "1" );
      println( "==>success to test use: " + JSON.stringify( selObj ) );

      /*【Test Point 4】 nest field $include=0*/
      var condObj = {};
      var selObj = { "ExtraField1.nest1.nest2.nest3": { "$include": 0 } };
      var ret = selMainQuery( cl, condObj, selObj );
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
