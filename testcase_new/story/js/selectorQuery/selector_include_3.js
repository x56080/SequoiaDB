/*******************************************************************************
*@Description : test selector: $include nest array
*@Example: record: {a:{b:{c:["d", "e"}},x:[{y:[{z:["m", "n"]}]}]}/
*          query: db.cs.cl.find({},{"a.b.c":{$include:1}}})
*@Modify list :
*               2015-01-29  xiaojun Hu  Change
*******************************************************************************/

function main( db )
{
   try
   {
      var recordNum = 1 ;
      var addRecord1 = { "nest1":{"nest2":{"nest3":["a","b"]}}} ;
      var addRecord2 = [{ "nest1":[{"nest2":[{"nest3":["a","b"]}]}]}] ;
      var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0, true, true, false,
                             "create colleciton in the begnning" ) ;
      // auto generate data
      selAutoGenData( cl, recordNum, addRecord1, addRecord2 ) ;
      println( "success to insert record: " + recordNum ) ;

      /*Test Point 1 $include=1/0, different field name*/
      var condObj = {} ;
      var selObj = { "ExtraField1.nest1.nest2.nest3": {"$include":1},
                     "GroupID": { "$include":1 },
                     "PrimariNode": { "$include":1 },
                     "Group.Service.Name": {"$include":1},
                     "ExtraField2.nest1.nest2.nest3": {"$include":0} } ;
      var ret = selMainQuery( cl, condObj, selObj ) ;
      //println( "query selector: " + JSON.stringify( selObj ) ) ;
      throw "ErrExcute" ;
   }
   catch( e )
   {
      if( -6 != e )
      {
         println( "failed to run test" + e ) ;
         throw e ;
      }
      println( "success to test $include mix use 1/0" ) ;
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
