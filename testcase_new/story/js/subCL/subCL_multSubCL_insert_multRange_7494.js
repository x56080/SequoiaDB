/*******************************************************************************
*@Description : attach multiple subCL, then insert, coverage of diffrent range
*@Modify List :
*   2015-03-09   xiaojun Hu   Init
*******************************************************************************/

function main ( db )
{
   var SUBCLNAME1 = CHANGEDPREFIX + "_1";
   var SUBCLNAME2 = CHANGEDPREFIX + "_2";
   var SUBCLNAME3 = CHANGEDPREFIX + "_3";
   var SUBCLNAME4 = CHANGEDPREFIX + "_4";
   db.setSessionAttr( { PreferedInstance: "M" } );
   commDropCL( db, COMMCSNAME, SUBCLNAME1, true, true,
      "clean collection" );
   commDropCL( db, COMMCSNAME, SUBCLNAME2, true, true,
      "clean collection" );
   commDropCL( db, COMMCSNAME, SUBCLNAME3, true, true,
      "clean collection" );
   commDropCL( db, COMMCSNAME, SUBCLNAME4, true, true,
      "clean collection" );
   commDropCL( db, COMMCSNAME, CHANGEDPREFIX, true, true,
      "clean collection" );
   println( "clean colleciton successful" );
   db.setSessionAttr( { PreferedInstance: "M" } );
   var clOptionObj = {
      ShardingKey: { no: 1 }, ShardingType: "range", ReplSize: 0,
      Compressed: true, IsMainCL: true
   };
   var mainCL = commCreateCLByOption( db, COMMCSNAME, CHANGEDPREFIX, clOptionObj,
      true, false, "create main collection" );
   var subCL1 = commCreateCL( db, COMMCSNAME, SUBCLNAME1, -1, true, true, false,
      "create sub collection 1" );
   var subCL2 = commCreateCL( db, COMMCSNAME, SUBCLNAME2, -1, true, true, false,
      "create sub collection 2" );
   var subCL3 = commCreateCL( db, COMMCSNAME, SUBCLNAME3, -1, true, true, false,
      "create sub collection 3" );
   var subCL4 = commCreateCL( db, COMMCSNAME, SUBCLNAME4, -1, true, true, false,
      "create sub collection 4" );
   println( "create main collection and sub collection successful" );
   mainCL.attachCL( COMMCSNAME + "." + SUBCLNAME1, {
      LowBound: { no: 1 },
      UpBound: { no: 2500 }
   } );
   mainCL.attachCL( COMMCSNAME + "." + SUBCLNAME2, {
      LowBound: { no: 2500 },
      UpBound: { no: 5000 }
   } );
   mainCL.attachCL( COMMCSNAME + "." + SUBCLNAME3, {
      LowBound: { no: 5000 },
      UpBound: { no: 7500 }
   } );
   mainCL.attachCL( COMMCSNAME + "." + SUBCLNAME4, {
      LowBound: { no: 7500 },
      UpBound: { no: 10000 }
   } );
   println( "attach sub collection successful" );
   // insert data
   for( var i = 1; i < 10000; ++i )
   {
      mainCL.insert( {
         no: i, "description": "testcase for main collection and sub " +
            "collection, testcase " + i
      } );
   }
   println( "insert 10000 records successful" );
   var explainQuery = mainCL.find( { $and: [{ no: { $gt: 5010 } }, { no: { $lt: 7490 } }] }
   ).explain( { Run: true } ).toArray();
   queryObj = JSON.parse( explainQuery );
   var clname = COMMCSNAME + "." + SUBCLNAME3;
   if( queryObj["SubCollections"][0]["Name"] != clname )
   {
      println( "expect cl name: " + clname );
      println( "actual explain query return: " );
      println( explainQuery );
      throw "wrong query data from sub collection";
   }
   println( "queryNumber: " + explainQuery );
   if( 2479 != queryObj["SubCollections"][0]["ReturnNum"] )
   {
      println( "expect query number : 2479" );
      println( "actual explain query : " );
      println( explainQuery );
      throw "wrong query number of data from sub collection";
   }
   // clean in the end
   commDropCL( db, COMMCSNAME, SUBCLNAME1, false, false,
      "clean collection" );
   commDropCL( db, COMMCSNAME, SUBCLNAME2, false, false,
      "clean collection" );
   commDropCL( db, COMMCSNAME, SUBCLNAME3, false, false,
      "clean collection" );
   commDropCL( db, COMMCSNAME, SUBCLNAME4, false, false,
      "clean collection" );
   commDropCL( db, COMMCSNAME, CHANGEDPREFIX, false, false,
      "clean collection" );
}

try 
{
   if( false != commIsStandalone( db ) ) 
   {
      println( "run mode is standalone" );
   }
   else 
   {
      main( db );
   }
} catch( e )
{
   throw e;
}
