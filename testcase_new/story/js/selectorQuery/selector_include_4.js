/*******************************************************************************
*@Description : test selector: $include when query condition field don't exist
*@Example: record: {a:[{b:[{c:["d", "e"]}]}]}
*          query: db.cs.cl.find({},{"a.b.x":{$include:1/0}})
*          query: db.cs.cl.find({},{"a.x.c":{$include:1/0}})
*          query: db.cs.cl.find({},{"x.b.c":{$include:1/0}})
*@Modify list :
*               2015-01-29  xiaojun Hu  Change
*******************************************************************************/

function main ( db )
{
   try
   {
      var recordNum = 100;
      var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, {}, true, false,
         "create colleciton in the begnning" );
      // auto generate data
      selAutoGenData( cl, recordNum );
      println( "success to insert record: " + recordNum );

      /*Test Point 1 > {"a.b.x":{$include:1/0}}, field "x" not exist*/
      var condObj = {};
      var selObj = { "Group.Service.NameXXX": { "$include": 1 } };
      var ret = selMainQuery( cl, condObj, selObj );
      println( "==>success to test use: " + JSON.stringify( selObj ) );
      selVerifyIncludeRet( ret, selObj, 1, "0" );
      var condObj = {};
      var selObj = { "Group.Service.NameXXX": { "$include": 0 } };
      var ret = selMainQuery( cl, condObj, selObj );
      println( "==>success to test use: " + JSON.stringify( selObj ) );
      selVerifyIncludeRet( ret, selObj, 1, "0" );

      /*Test Point 1 > {"a.x.c":{$include:1/0}}, field "x" not exist*/
      var condObj = {};
      var selObj = { "Group.ServiceXXX.Name": { "$include": 1 } };
      var ret = selMainQuery( cl, condObj, selObj );
      selVerifyIncludeRet( ret, selObj, 1, "0" );
      println( "==>success to test use: " + JSON.stringify( selObj ) );
      var condObj = {};
      var selObj = { "Group.ServiceXXX.Name": { "$include": 0 } };
      var ret = selMainQuery( cl, condObj, selObj );
      selVerifyIncludeRet( ret, selObj, 1, "0" );
      println( "==>success to test use: " + JSON.stringify( selObj ) );

      /*Test Point 1 > {"x.b.c":{$include:1/0}}, field "x" not exist*/
      var condObj = {};
      var selObj = { "GroupXXX.Service.Name": { "$include": 1 } };
      var ret = selMainQuery( cl, condObj, selObj );
      selVerifyIncludeRet( ret, selObj, 1, "0" );
      println( "==>success to test use: " + JSON.stringify( selObj ) );
      var condObj = {};
      var selObj = { "GroupXXX.Service.Name": { "$include": 0 } };
      var ret = selMainQuery( cl, condObj, selObj );
      selVerifyIncludeRet( ret, selObj, 1, "0" );
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
