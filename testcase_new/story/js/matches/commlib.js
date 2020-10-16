/*******************************************************************************
*@Description : matches testcase common functions and varialb
*@Modify list :
*              2016-5-16 xiaoni huang
*******************************************************************************/
import( "../lib/main.js" );

function readyCL ( clName )
{
   println( "\n---Begin to create CL." );

   commDropCL( db, COMMCSNAME, clName, true, true,
      "Failed to drop CL in the pre-condition." );

   var cl = commCreateCL( db, COMMCSNAME, clName, {}, true, false,
      "Failed to create CL." );
   return cl;
}

function cleanCL ( clName )
{
   println( "\n---Begin to drop CL." );

   commDropCL( db, COMMCSNAME, clName, false, false,
      "Failed to drop CL in the end-condition" );
}

function checkRec ( rc, expRecs )
{
   //get actual records to array
   var actRecs = [];
   while( rc.next() )
   {
      actRecs.push( rc.current().toObj() );
   }

   //check count
   if( actRecs.length !== expRecs.length )
   {
      throw new Error( "\nActual recs in cl= " + JSON.stringify( actRecs ) + "\n\nExpect recs= " + JSON.stringify( expRecs ) );
   }

   //check every records every fields
   for( var i in expRecs )
   {
      var actRec = actRecs[i];
      var expRec = expRecs[i];
      for( var f in expRec )
      {
         if( JSON.stringify( actRec[f] ) !== JSON.stringify( expRec[f] ) )
         {
            throw new Error ("\nError occurs in " + ( parseInt( i ) + 1 ) + "th record, in field '" + f + "'\nactual recs in cl= " + JSON.stringify( actRecs ) + "\n\nexpect recs= " + JSON.stringify( expRecs ));
         }
      }
   }
}

function idxAutoGenData ( cl, insertNum )
{
   if( undefined == insertNum ) { insertNum = 1000; }
   try
   {
      var record = [];
      for( var i = 0; i < insertNum; ++i )
      {
         record.push( {
            "no": i, "no1": i * 2, "no2": i * 3,
            "obj_id": { "$oid": "123abcd00ef12358902300ef" },
            "subobj": { "obj": { "val": "sub" } },
            "string": "西边个喇嘛，东边个哑巴",
            "array": [i + "arr" + i, 5 * i, 2 * i + "ARR" + i, "arrayIndex"], "no3": 4 * i
         } );
      }
      cl.insert( record );
      cnt = 0;
      while( insertNum != cl.count() && cnt < 1000 )
      {
         ++cnt;
         sleep( 2 );
      }
      if( insertNum != cl.count() )
         throw new Error( "Expect insert number: " + insertNum + ", actual: " + cl.count() );
   }
   catch( e )
   {
      throw new Error ( "failed to insert data to db, rc = " + e );
   }
}

function idxQueryCheck ( cl, queryCond, verifyNum, idxName )
{
   try
   {
      var query = cl.find( queryCond ).explain( { Run: true } ).toArray();
      var queryObj = eval( "(" + query + ")" );
      /*
            if( "tbscan" == queryObj.ScanType )
            {
               println( "expect idxscan, actual: " + queryObj.ScanType ) ;
               throw "ErrorScanType" ;
            }
            if( idxName != queryObj.IndexName )
            {
               println( "expect index name: " + idxName + ", actual: " + queryObj.IndexName ) ;
               throw "ErrorIdxName" ;
            }
      */
      if( verifyNum != queryObj.ReturnNum )
      {
         throw new Error ("ErrorvReturnNum, expect number: " + verifyNum + ", actual: " + queryObj.ReturnNum);
      }
   }
   catch( e )
   {
      throw new Error ( "failed to inspect: " + e );
   }
}

function checkExplain ( rc, expIdxName )
{
   var plan = rc.explain().current().toObj();
   var expScanType = "ixscan";
   if( expIdxName == "" ) { expScanType = "tbscan"; }

   if( plan.ScanType !== expScanType )
   {
      throw new Error ("checkExplain() query.explain().ScanType, \nExpScanType: " + expScanType + "  actScanType: " + plan.ScanType );
   }
   if( plan.IndexName !== expIdxName )
   {
      throw new Error ("checkExplain() query.explain().IndexName, \nExpIdxName: " + expIdxName + "  ActIndexName: " + plan.IndexName );
   }
}
