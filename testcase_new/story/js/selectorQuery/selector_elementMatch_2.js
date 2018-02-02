/*******************************************************************************
*@Description : $elemMatch/$elemMatchOne
*@Example: record: {"field":[{a:"b"},{a:"c"},{a:"d"},{a:"b"},{a:"f"}]
*          query: db.cs.cl.find({},{"field":{$elemMatch:{a:"b"}}})
*          query: db.cs.cl.find({},{"field":{$elemMatchOne:{a:"b"}}})
*@Modify list :
*               2015-01-29  xiaojun Hu  Change
*******************************************************************************/

function main( db )
{
   try
   {
      var recordNum = 1 ;
      var addRecord1 = [{name:"ZhangSan", age:18 },
                       {name:"WangErmazi", age:19 },
                       {name:"lucy", age:20 },
                       {name:"alex", age:18 },
                       {name:"shanven", age:18 }] ;
      //var addRecord2 = {"nest1":{"nest2":{"nest3":{"nest4":"element match query"}}}} ;
      var addRecord2 = {"nestObj":"element match query"} ;
      var addRecord3 = [{"nestArr1":[{"nestArr2":[{"nestArr3":["abc",158,"elementMatch","中文"]}]}]}] ;
      var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0, true, true, false,
                             "create colleciton in the begnning" ) ;
      // auto generate data
      selAutoGenData( cl, recordNum, addRecord1, addRecord2, addRecord3 ) ;
      println( "success to insert record: " + recordNum ) ;

      /*【Test Point 1】 $elemMatch: query field isn't array[abnormal]*/
      var condObj = {} ;
      var selObj = { "ExtraField2":{"$elemMatch": {"nestObj":"element match query"}}} ;
      var ret = selMainQuery( cl, condObj, selObj ) ;
      // verify
      var retObj = JSON.parse( ret ) ;
      var cnt = retObj["ExtraField2"] ;
      if( "undefined" != typeof( cnt ) )
      {
         println( "expect count: 3, actual count: " + cnt  ) ;
         throw "ErrCountRecord" ;
      }
      println( "==>success to test use: " + JSON.stringify( selObj ) ) ;

      /*【Test Point 2】 $elemMatchOne: query field isn't array[abnormal]*/
      var condObj = {} ;
      var selObj = { "ExtraField2":{"$elemMatchOne": {"nestObj":"element match query"}}} ;
      var ret = selMainQuery( cl, condObj, selObj ) ;
      // verify
      var retObj = JSON.parse( ret ) ;
      var cnt = retObj["ExtraField2"] ;
      if( "undefined" != typeof( cnt ) )
      {
         println( "expect count: 3, actual count: " + cnt  ) ;
         throw "ErrCountRecord" ;
      }
      println( "==>success to test use: " + JSON.stringify( selObj ) ) ;

      /*【Test Point 3】 $elemMatch: nest array, array element isn't object*/
      var condObj = {} ;
      var selObj = { "ExtraField3.nestArr1.nestArr2.nestArr3": {"$elemMatch": 158}} ;
      try
      {
         var ret = selMainQuery( cl, condObj, selObj ) ;
         throw "Should'n Throw Exception" ;
      }
      catch( e )
      {
         if( -6 != e )
         {
            println( "failed to test the query : " + e ) ;
            throw e ;
         }
      }

      /*【Test Point 4】 $elemMatchOne: nest array, array element isn't object*/
      var condObj = {} ;
      var selObj = { "ExtraField3.nestArr1.nestArr2.nestArr3": {"$elemMatch": 158}} ;
      try
      {
         var ret = selMainQuery( cl, condObj, selObj ) ;
         throw "Should'n Throw Exception" ;
      }
      catch( e )
      {
         if( -6 != e )
         {
            println( "failed to test the query : " + e ) ;
            throw e ;
         }
      }
   }
   catch( e )
   {
      println( "==>failed to test use: " + JSON.stringify( selObj ) ) ;
      throw e ;
   }
}

// Run Main
try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
               "drop collection in the begining" ) ;
   main( db ) ;
   //commDropCL( db, COMMCSNAME, COMMCLNAME, false, false,
   //            "drop collection in the end") ;
}
catch( e )
{
   //commDropCL( db, COMMCSNAME, COMMCLNAME, false, false,
   //            "drop collection in the end") ;
   throw e ;
}
