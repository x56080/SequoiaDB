
db = new Sdb("localhost", 11810)
// 其中 10000/11000/12000 为 同一个主内的三个节点
db11 = new Sdb("localhost", 10000)
db12 = new Sdb("localhost", 11000)
db13 = new Sdb("localhost", 12000)

println( db11.IBSFLOW.FILE_2020.count() )
println( db12.IBSFLOW.FILE_2020.count() )
println( db13.IBSFLOW.FILE_2020.count() )

println( db.IBSFLOW.FILE_2020.find({"site_list.$0.site_id":{$isnull:1}}).count() )
println( db11.IBSFLOW.FILE_2020.find({"site_list.$0.site_id":{$isnull:1}}).count() )
println( db12.IBSFLOW.FILE_2020.find({"site_list.$0.site_id":{$isnull:1}}).count() )
println( db13.IBSFLOW.FILE_2020.find({"site_list.$0.site_id":{$isnull:1}}).count() )

println( db.IBSFLOW.FILE_2020.find({site_list:[]}).count() )
println( db11.IBSFLOW.FILE_2020.find({site_list:[]}).count() )
println( db12.IBSFLOW.FILE_2020.find({site_list:[]}).count() )
println( db13.IBSFLOW.FILE_2020.find({site_list:[]}).count() )
