/******************************************************************************
*@Description : test find special decimal value with update symbol
*               $inc $addtoset
*               seqDB-13995:使用更新符更新特殊decimal值           
*@author      : Liang XueWang 
******************************************************************************/
main() ;

function main()
{
   var docs = [ { a: { $decimal: "MAX" } },
                { a: { $decimal: "MIN" } },
                { a: { $decimal: "NaN" } } ] ;
                
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" ) ;
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0 ) ;
   insertData( cl, docs ) ;
   
   testUpdateData( cl, { a: { $inc: 1 } }, -6 ) ;
   
   testAddToSet( cl ) ;
}

function testUpdateData( cl, rule, errno )
{
   try
   {
      cl.update( rule ) ;
      throw 0 ;
   }
   catch( e )
   {
      if( e !== errno )
      {
         throw buildException( "testUpdateData", e, "update", errno, e ) ;
      }
   }
}

function testAddToSet( cl )
{
   var doc = { b: [] } ;
   insertData( cl, doc ) ;
   cl.update( { $addtoset: { b: [ { "$decimal": "MAX" },
                                  { "$decimal": "MIN" },
                                  { "$decimal": "NaN" } ] } },
              { b: { $exists: 1 } } ) ;
   var cursor = findData( cl, { b: { $exists: 1 } } ) ;
   var expRecs = [ { b: [ { "$decimal": "MIN" },
                          { "$decimal": "NaN" },
                          { "$decimal": "MAX" } ] } ] ;
   checkRec( cursor, expRecs ) ;
}