/*******************************************************************************
*@Description : test selector: $default when query condition field don't exist
*@Test Point: 1. {"Group.Service.NameXXX":{"$default":"NotExist"}}
*                [field NameXXX not exist]
*             2. {"Group.ServiceXXX.Name":{"$default":"NotExist"}}
*                [field ServiceXXX not exist]
*             3. {"GroupXXX.Service.Name":{"$default":"NotExist"}}
*                [field GroupXXX not exist]
*             4. multi field use $default in one record
*@Modify list :
*               2015-01-29  xiaojun Hu  Change
*******************************************************************************/

function main ( db )
{
   try
   {
      var recordNum = 1;
      var addRecord1 = { "nest1": { "nest2": { "nest3": ["a", "b"] } } };
      var addRecord2 = [{ "nest1": [{ "nest2": [{ "nest3": ["a", "b"] }] }] }];
      var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0, true, true, false,
         "create colleciton in the begnning" );
      // auto generate data
      selAutoGenData( cl, recordNum, addRecord1, addRecord2 );
      println( "success to insert " + recordNum + " records" );

      /*【Test Point 1】 field "NameXXX" not exist*/
      var condObj = {};
      var selObj = { "Group.Service.NameXXX": { "$default": "NotExist" } };
      var ret = selMainQuery( cl, condObj, selObj );
      // verify
      var retObj = JSON.parse( ret );
      retObj = retObj["Group"][0]["Service"];
      for( var i in retObj )
      {
         if( "NotExist" != retObj[i]["NameXXX"] )
         {
            println( "query return record: " + ret );
            throw "ErrQueryRecord1";
         }
      }
      println( "==>success to test, use: " + JSON.stringify( selObj ) );

      /*【Test Point 2】 field "ServiceXXX" not exist*/
      var condObj = {};
      var selObj = { "Group.ServiceXXX.Name": { "$default": "NotExist" } };
      var ret = selMainQuery( cl, condObj, selObj );
      // verify
      var retObj = JSON.parse( ret );
      if( "NotExist" != retObj["Group"][0]["ServiceXXX"]["Name"] )
      {
         println( "query return record: " + ret );
         throw "ErrQueryRecord1";
      }
      println( "==>success to test, use: " + JSON.stringify( selObj ) );

      /*【Test Point 3】 field "GroupXXX" not exist*/
      var condObj = {};
      var selObj = { "GroupXXX.Service.Name": { "$default": "NotExist" } };
      var ret = selMainQuery( cl, condObj, selObj );
      // verify
      var retObj = JSON.parse( ret );
      if( "NotExist" != retObj["GroupXXX"]["Service"]["Name"] )
      {
         println( "query return record: " + ret );
         throw "ErrQueryRecord1";
      }
      println( "==>success to test, use: " + JSON.stringify( selObj ) );

      /*【Test Point 4】 multi field use $default */
      var condObj = {};
      var selObj = {
         "ExtraField1.nest1.nest2.nest3": { "$default": 1 },
         "Group.HostName": { "$default": 1 },
         "Group.dbpath": { "$default": 1 },
         "Group.NodeID": { "$default": 1 },
         "Group.Service.Name": { "$default": 1 },
         "PrimaryNode": { "$default": 1 },
         "ExtraField2.nest1.nest2.nest3": { "$default": 1 }
      };
      var ret = selMainQuery( cl, condObj, selObj );
      // need verify
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
