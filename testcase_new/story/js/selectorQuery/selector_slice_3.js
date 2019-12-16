/*******************************************************************************
*@Description : $slice
*               record: ["a","b","c","d","e","f","g","h", ... ,"w","x","y","z"]
*               query: {"$slice": [19, 0]}
*               query: {"$slice": [19, -7]}    [abnormal test]
*               query: {"$slice": [9,70]}
*               query: {"$slice": {"HostName": "Host_1"}}    [abnormal test]
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
      var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, {}, true, false,
         "create colleciton in the begnning" );
      // auto generate data
      selAutoGenData( cl, recordNum, addRecord );
      println( "success to insert record: " + recordNum );

      /*【Test Point 1】 {"$slice": [19, 0]}*/
      var condObj = {};
      var selObj = { "ExtraField1": { "$slice": [19, 0] } };
      var ret = selMainQuery( cl, condObj, selObj );
      // verify
      var retObj = JSON.parse( ret );
      if( "" != retObj["ExtraField1"] && 1 == recordNum )
      {
         println( "record: " + cl.find() + "query record: " + ret );
         println( "query field value: " + retObj["ExtraField1"] );
         throw "ErrVerify1-$Slice";
      }
      selVerifyNonSelectorObj( cl, ret, condObj, selObj );
      println( "==>success to test use: " + JSON.stringify( selObj ) );

      /*【Test Point 2】 abnormal: {"$slice": [19, -7]}*/
      var condObj = {};
      var selObj = { "ExtraField1": { "$slice": [19, -7] } };
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

      /*【Test Point 3】 {"ExtraField1":{"$slice": [9,70]}}*/
      var condObj = {};
      var selObj = { "ExtraField1": { "$slice": [9, 70] } };
      var ret = selMainQuery( cl, condObj, selObj );
      // verify
      var retObj = JSON.parse( ret );
      if( "j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z" != retObj["ExtraField1"]
         && 1 == recordNum )
      {
         println( "record: " + cl.find() + "query record: " + ret );
         println( "query field value: " + retObj["ExtraField1"] );
         throw "ErrVerify3-$Slice";
      }
      selVerifyNonSelectorObj( cl, ret, condObj, selObj );
      println( "==>success to test use: " + JSON.stringify( selObj ) );

      /*【Test Point 4】 abnormal: {"$slice": {"HostName": "Host_1"}]}*/
      var condObj = {};
      var selObj = { "ExtraField1": { "$slice": { "HostName": "Host_1" } } };
      try
      {
         var ret = selMainQuery( cl, condObj, selObj );
         throw "Wrong,Should'n Be Successful";
      }
      catch( e )
      {
         if( -6 != e )
         {
            println( "failed to excute selector query by using: " +
               JSON.stringify( selObj ) +
               ", rc = " + e );
            throw e;
         }
         println( "==>success to test use: " + JSON.stringify( selObj ) );
      }

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
