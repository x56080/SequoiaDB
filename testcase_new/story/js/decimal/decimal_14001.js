/******************************************************************************
*@Description : test illegal special decimal value
*               seqDB-14001:特殊decimal值参数校验         
*@author      : Liang XueWang 
******************************************************************************/
main() ;

function main()
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" ) ;
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0 ) ;
   
   var legalDocs = [ { a: { $decimal: "mAx" } },
                     { a: { $decimal: "MiN" } },
                     { a: { $decimal: "-Inf" } },
                     { a: { $decimal: "iNF" } },
                     { a: { $decimal: "nan" } } ] ;
   insertData( cl, legalDocs ) ;
   
   var illegalDocs = [ { a: { $decimal: "MAX1" } },
                       { a: { $decimal: "1Max" } },
                       { a: { $decimal: "MMAX" } },
                       { a: { $decimal: "Maxx" } },
                       { a: { $decimal: "maax" } },
                       { a: { $decimal: " max" } },
                       { a: { $decimal: "m ax" } },
                       { a: { $decimal: "ma x" } },
                       { a: { $decimal: "max " } },
                       { a: { $decimal: "abc" } } ] ;
   for( var i = 0;i < illegalDocs.length;i++ )
   {
      try
      {
         cl.insert( illegalDocs[i] ) ;
         throw 0 ;
      }
      catch( e )
      {
         if( e !== -6 )
         {
            throw buildException( "main", e, "insert illegal decimal i=" + i, -6, e ) ;
         }
      }
   }
}