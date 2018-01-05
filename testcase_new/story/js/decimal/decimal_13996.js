/******************************************************************************
*@Description : test find special decimal value with aggregate symbol
*               $sort $group($addtoset $max $min $avg $sum)
*               seqDB-13996:使用聚集符查询特殊decimal值           
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
   
   println( "test $sort" ) ;
   var cursor = aggregate( cl, { $sort: { a: 1 } } ) ;
   var expRecs = [ { a: { $decimal: "MIN" } },
                   { a: { $decimal: "NaN" } },
                   { a: { $decimal: "MAX" } } ] ;
   checkRec( cursor, expRecs ) ;
   
   // insert MAX MIN NAN to diff group to test $addtoset
   deleteData( cl ) ;
   docs = [ { gid: 3, a: { $decimal: "MAX" } },
            { gid: 2, a: { $decimal: "NaN" } },
            { gid: 1, a: { $decimal: "MIN" } } ] ;
   insertData( cl, docs ) ;
   
   println( "test $addtoset" ) ;
   cursor = aggregate( cl, { $group: { _id: "$gid", b: { $addtoset: "$a" } } } ) ;
   expRecs = [ { b: [ { $decimal: "MIN" } ] },
               { b: [ { $decimal: "NaN" } ] },
               { b: [ { $decimal: "MAX" } ] } ] ;
   checkRec( cursor, expRecs ) ;
   
   // insert MAX MIN NAN to same group to test $max $min $avg $sum
   deleteData( cl ) ;
   docs = [ { gid: 1, a: { $decimal: "MAX" } },
            { gid: 1, a: { $decimal: "NaN" } },
            { gid: 1, a: { $decimal: "MIN" } } ] ;
   insertData( cl, docs ) ; 
           
   println( "test $max" ) ;
   cursor = aggregate( cl, { $group: { _id: "$gid", b: { $max: "$a" } } } ) ;
   expRecs = [ { b: { $decimal: "MAX" } } ] ;
   checkRec( cursor, expRecs ) ;
   
   println( "test $min" ) ;
   cursor = aggregate( cl, { $group: { _id: "$gid", b: { $min: "$a" } } } ) ;
   expRecs = [ { b: { $decimal: "MIN" } } ] ;
   checkRec( cursor, expRecs ) ;
   
   println( "test $avg" ) ;
   cursor = aggregate( cl, { $group: { _id: "$gid", b: { $avg: "$a" } } } ) ;
   expRecs = [ { b: { $decimal: "NaN" } } ] ;
   checkRec( cursor, expRecs ) ;
   
   println( "test $sum" ) ;
   cursor = aggregate( cl, { $group: { _id: "$gid", b: { $sum: "$a" } } } ) ;
   expRecs = [ { b: { $decimal: "NaN" } } ] ;
   checkRec( cursor, expRecs ) ;
}