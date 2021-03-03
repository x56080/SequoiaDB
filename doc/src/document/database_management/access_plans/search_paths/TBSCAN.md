TBSCAN 的推演公式将展示以下信息：

| 字段名         | 类型   | 描述 |
| -------------- | ------ | ---- |
| MthSelectivity | 浮点型 | 估算的 TBSCAN 使用匹配符进行过滤的选择率 |
| MthCPUCost     | 整型   | 估算的 TBSCAN 使用匹配符过滤一个记录的 CPU 代价 |
| IOCost         | 数组   | 估算的 TBSCAN 的 IO 代价的公式及计算过程<br>NeedEvalIO 为 false 不需要计算<br>即各个数据页进行顺序扫描的代价总和<br>公式为：```SeqReadIOCostUnit * Pages * ( PageSize / PageUnit )``` |
| CPUCost        | 数组   | 估算的 TBSCAN 的 CPU 代价的公式及计算过程<br>即各个记录从数据页中提取并进行匹配符过滤的代价总和<br>公式为：```Records * ( RecExtractCPUCost + MthCPUCost )``` |
| StartCost      | 数组   | 估算的 TBSCAN 的启动代价（内部表示）<br>公式为：```TBScanStartCost``` |
| RunCost        | 数组   | 估算的 TBSCAN 的运行代价（内部表示）<br>公式为：```IOCPURate * IOCost + CPUCost``` |
| TotalCost      | 数组   | 估算的 TBSCAN 的总代价（内部表示）<br>公式为：```StartCost + RunCost``` |
| OutputRecords  | 数组   | 估算的 TBSCAN 的输出记录个数<br>公式为：```max( 1, ceil( Records * MthSelectivity ) )``` |

**示例**

```
"ScanNode": {
  "MthSelectivity": 1,
    "MthCPUCost": 0,
    "IOCost": [
      "SeqReadIOCostUnit * Pages * ( PageSize / PageUnit )",
      "1 * 74 * ( 65536 / 4096 ) ",
      1184
  ],
  "CPUCost": [
    "Records * ( RecExtractCPUCost + MthCPUCost )",
    "100000 * ( 4 + 0 ) ",
    400000
  ],
  "StartCost": [
    "TBScanStartCost",
    "0",
    0
  ],
  "RunCost": [
    "IOCPURate * IOCost + CPUCost",
    "2000 * 1184 + 400000",
    2768000
  ],
  "TotalCost": [
    "StartCost + RunCost",
    "0 + 2768000",
    2768000
  ],
  "OutputRecords": [
    "max( 1, ceil( Records * MthSelectivity ) )",
    "max( 1, ceil( 100000 * 1 ) )",
    100000
  ]
}
```
