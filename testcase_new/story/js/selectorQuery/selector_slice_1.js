/*******************************************************************************
*@Description : $slice
*               record: ["a","b","c","d","e","f", ... , "v","w","x","y","z"]
*               query: {"$slice": 6}  [ get the previous six array elements ]
*               query: {"$slice": -7}  [ get the last seven array elements ]
*               query: {"$slice": [10,10]}
*               query: {"$slice": [-20,19]}
*@Modify list :
*               2015-01-26  xiaojun Hu  Init
*******************************************************************************/

function main ( db )
{
   try
   {
      var recordNum = 1;
      var addRecord = ["a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m",
         "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z"];
      var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0, true, true, false,
         "create colleciton in the begnning" );
      // auto generate data
      selAutoGenData( cl, recordNum, addRecord );
      println( "success to insert record: " + recordNum );

      /*【Test Point 1.1】 {"$slice": 6}*/
      var condObj = {};
      var selObj = { "ExtraField1": { "$slice": 6 } };
      var ret = selMainQuery( cl, condObj, selObj );
      // verify
      var retObj = JSON.parse( ret );
      if( "a,b,c,d,e,f" != retObj["ExtraField1"] && 1 == recordNum )
      {
         println( "record: " + cl.find() + "query record: " + ret );
         println( "query field value: " + retObj["ExtraField1"] );
         throw "ErrVerify1-$Slice";
      }
      selVerifyNonSelectorObj( cl, ret, condObj, selObj );
      println( "==>success to test use: " + JSON.stringify( selObj ) );
      /*【Test Point 1.2】 {"$slice": -7}*/
      var condObj = {};
      var selObj = { "ExtraField1": { "$slice": -7 } };
      var ret = selMainQuery( cl, condObj, selObj );
      // verify
      var retObj = JSON.parse( ret );
      if( "t,u,v,w,x,y,z" != retObj["ExtraField1"] && 1 == recordNum )
      {
         println( "record: " + cl.find() + "query record: " + ret );
         println( "query field value: " + retObj["ExtraField1"] );
         throw "ErrVerify2-$Slice";
      }
      selVerifyNonSelectorObj( cl, ret, condObj, selObj );
      println( "==>success to test use: " + JSON.stringify( selObj ) );

      /*【Test Point 2.1】 {"$slice": [10,10]}*/
      var condObj = {};
      var selObj = { "ExtraField1": { "$slice": [10, 10] } };
      var ret = selMainQuery( cl, condObj, selObj );
      // verify
      var retObj = JSON.parse( ret );
      if( "k,l,m,n,o,p,q,r,s,t" != retObj["ExtraField1"] && 1 == recordNum )
      {
         println( "record: " + cl.find() + "query record: " + ret );
         println( "query field value: " + retObj["ExtraField1"] );
         throw "ErrVerify2-$Slice";
      }
      selVerifyNonSelectorObj( cl, ret, condObj, selObj );
      println( "==>success to test use: " + JSON.stringify( selObj ) );
      /*【Test Point 2.2】 {"$slice": [-20,19]}*/
      var condObj = {};
      var selObj = { "ExtraField1": { "$slice": [-20, 19] } };
      var ret = selMainQuery( cl, condObj, selObj );
      // verify
      var retObj = JSON.parse( ret );
      if( "g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y" != retObj["ExtraField1"] &&
         1 == recordNum )
      {
         println( "record: " + cl.find() + "query record: " + ret );
         println( "query field value: " + retObj["ExtraField1"] );
         throw "ErrVerify2-$Slice";
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
