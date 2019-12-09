/*******************************************************************************
*@Description : $elemMatch/$elemMatchOne
*@Example: record: {"field":[{a:"b"},{a:"c"},{a:"d"},{a:"b"},{a:"f"}]
*          query: db.cs.cl.find({},{"field":{$elemMatch:{a:"b"}}})
*          query: db.cs.cl.find({},{"field":{$elemMatchOne:{a:"b"}}})
*@Modify list :
*               2015-02-04  xiaojun Hu  Change
*******************************************************************************/

function main ( db )
{
   try
   {
      var recordNum = 1;
      var addRecord = [{ name: "ZhangSan", age: 18 },
      { name: "WangErmazi", age: 19 },
      { name: "lucy", age: 20 },
      { name: "alex", age: 18 },
      { name: "shanven", age: 18 }];
      var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0, true, true, false,
         "create colleciton in the begnning" );
      // auto generate data
      selAutoGenData( cl, recordNum, addRecord );
      println( "success to insert record: " + recordNum );

      /*【Test Point 1】 $elemMatch: normal operation*/
      var condObj = {};
      var selObj = { "ExtraField1": { "$elemMatch": { age: 18 } } };
      var ret = selMainQuery( cl, condObj, selObj );
      // verify
      var retObj = JSON.parse( ret );
      var cnt = retObj["ExtraField1"].length;
      for( var i = 0; i < cnt; ++i )
      {
         if( selObj["ExtraField1"]["$elemMatch"]["age"] !=
            retObj["ExtraField1"][i]["age"] )
         {
            println( "return record: " + ret );
            throw "ErrReturnRecord";
         }
      }
      if( 3 != cnt )
      {
         println( "expect count: 3, actual count: " + cnt );
         throw "ErrCountRecord";
      }
      selVerifyNonSelectorObj( cl, ret, condObj, selObj );
      println( "==>success to test use: " + JSON.stringify( selObj ) );

      /*【Test Point 2】 $elemMatchOne: normal operation*/
      var condObj = {};
      var selObj = { "ExtraField1": { "$elemMatchOne": { age: 18 } } };
      var ret = selMainQuery( cl, condObj, selObj );
      // verify
      var retObj = JSON.parse( ret );
      var cnt = retObj["ExtraField1"].length;
      for( var i = 0; i < cnt; ++i )
      {
         if( selObj["ExtraField1"]["$elemMatchOne"]["age"] !=
            retObj["ExtraField1"][i]["age"] )
         {
            println( "return record: " + ret );
            throw "ErrReturnRecord";
         }
      }
      if( 1 != cnt )
      {
         println( "expect count: 3, actual count: " + cnt );
         throw "ErrCountRecord";
      }
      selVerifyNonSelectorObj( cl, ret, condObj, selObj );
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
