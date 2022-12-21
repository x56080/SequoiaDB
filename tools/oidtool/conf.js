/**
 * Configure file for sdboidtool.js
 */
// 连接信息：
var coord_hostname = "192.168.16.37" ;
var coord_port     = "11810" ;
var user           = "" ;
var passwd         = "" ;

// 运行时间:
// 当选择进行数据检测或者修复时（ACTION = "check"/"repair"），
// 任务将在如下时间段内进行（最后一次启动任务的时间不晚于 end_timestamp）
var begin_timestamp = "2022-12-14-22.00.00.000000" ;
var end_timestamp   = "2022-12-15-06.00.00.000000" ;