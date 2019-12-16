/*******************************************************************************
*@Description : $elemMatch/$elemMatchOne
*@Example: record: {"field":[{a:"b"},{a:"c"},{a:"d"},{a:"b"},{a:"f"}]
*          query: db.cs.cl.find({},{"field":{$elemMatch:{a:"b"}}})
*          query: db.cs.cl.find({},{"field":{$elemMatchOne:{a:"b"}}})
*@Modify list :
*               2015-01-29  xiaojun Hu  Change
*******************************************************************************/

function main ( db )
{
   try
   {
      var recordNum = 1;
      var addRecord1 = [{ name: "ZhangSan", age: 18 },
      { name: "WangErmazi", age: 19 },
      { name: "lucy", age: 20 },
      { name: "alex", age: 18 },
      { name: "shanven", age: 18 }];
      var addRecord2 = { "nest1": { "nest2": { "nest3": { "nest4": "element match query" } } } };
      var addRecord3 = [{
         "nest1": [{
            "nest2": [{
               "nest3":
                  [{ "abc": "中文" }, { "abc": "中文" }, { "abc": "中文" }]
            }]
         }]
      }];
      var addRecord4 = { "nest1": { "nest2": { "nest3": ["nest4", 123, "element match query"] } } };
      var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, {}, true, false,
         "create colleciton in the begnning" );
      // auto generate data
      selAutoGenData( cl, recordNum, addRecord1, addRecord2, addRecord3, addRecord4 );
      println( "success to insert record: " + recordNum );

      /*【Test Point 1】 $elemMatch: selector field value is array*/
      var condObj = {};
      var selObj = {
         "ExtraField4.nest1.nest2.nest3":
            { "$elemMatch": ["nest4", 123, "element match query"] }
      };
      try
      {
         var ret = selMainQuery( cl, condObj, selObj );
         throw "Should not be run successful";
      }
      catch( e )
      {
         if( -6 != e )
         {
            println( "failed to test selctor query, rc = " + e );
            throw e;
         }
      }
      println( "==>success to test use: " + JSON.stringify( selObj ) );
      /*【Test Point 2】 $elemMatchOne: selector field value is array*/
      var condObj = {};
      var selObj = {
         "ExtraField4.nest1.nest2.nest3":
            { "$elemMatchOne": ["nest4", 123, "element match query"] }
      };
      try
      {
         var ret = selMainQuery( cl, condObj, selObj );
         throw "Should not be run successful";
      }
      catch( e )
      {
         if( -6 != e )
         {
            println( "failed to test selctor query, rc = " + e );
            throw e;
         }
      }
      println( "==>success to test use: " + JSON.stringify( selObj ) );


      /*【Test Point 3】 $elemMatch: selector field value is object*/
      var condObj = {};
      var selObj = {
         "ExtraField3.nest1.nest2":
            { "$elemMatch": { "nest3": { "abc": "中文" } } }
      };
      var ret = selMainQuery( cl, condObj, selObj );
      // verify
      var retObj = JSON.parse( ret );
      var cnt = retObj["ExtraField3"][0]["nest1"][0]["nest2"][0]["nest3"];
      for( var i = 0; i < cnt.length; ++i )
      {
         if( "中文" != cnt[0]["abc"] )
         {
            println( "return record: " + ret );
            throw "ErrReturnRecord";
         }
      }
      if( 3 != cnt.length )
      {
         println( "expect count: 1, actual count: " + cnt.length );
         throw "ErrCountRecord";
      }
      selVerifyNonSelectorObj( cl, ret, condObj, selObj );
      println( "==>success to test use: " + JSON.stringify( selObj ) );

      /*【Test Point 4】 $elemMatchOne: selector field value is object*/
      var condObj = {};
      var selObj = {
         "ExtraField3.nest1.nest2":
            { "$elemMatchOne": { "nest3": { "abc": "中文" } } }
      };
      var ret = selMainQuery( cl, condObj, selObj );
      // verify
      var retObj = JSON.parse( ret );
      var cnt = retObj["ExtraField3"][0]["nest1"][0]["nest2"][0]["nest3"];
      for( var i = 0; i < cnt.length; ++i )
      {
         if( "中文" != cnt[0]["abc"] )
         {
            println( "return record: " + ret );
            throw "ErrReturnRecord";
         }
      }
      if( 3 != cnt.length )
      {
         println( "expect count: 1, actual count: " + cnt.length );
         throw "ErrCountRecord";
      }
      selVerifyNonSelectorObj( cl, ret, condObj, selObj );
      println( "==>success to test use: " + JSON.stringify( selObj ) );


      /*【Test Point 5】 $elemMatch: selector field value is nest object*/
      var condObj = {};
      var selObj = {
         "ExtraField2.nest1.nest2":
            { "$elemMatch": { "nest3": { "nest4": "element match query" } } }
      };
      var ret = selMainQuery( cl, condObj, selObj );
      selVerifyNonSelectorObj( cl, ret, condObj, selObj );
      println( "==>success to test use: " + JSON.stringify( selObj ) );

      /*【Test Point 6】 $elemMatchOne: selector field value is nest object*/
      var condObj = {};
      var selObj = {
         "ExtraField2.nest1.nest2":
            { "$elemMatch": { "nest3": { "nest4": "element match query" } } }
      };
      var ret = selMainQuery( cl, condObj, selObj );
      selVerifyNonSelectorObj( cl, ret, condObj, selObj );
      println( "==>success to test use: " + JSON.stringify( selObj ) );
   }
   catch( e )
   {
      println( "==>failed to test use: " + JSON.stringify( selObj ) );
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
