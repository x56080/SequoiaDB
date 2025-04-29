

TBSCAN 的推演公式将展示以下信息：

| 字段名         | 类型   | 描述 |
| -------------- | ------ | ---- |
| MthSelectivity | double | 估算的 TBSCAN 使用匹配符进行过滤的选择率 |
| ScanRate       | double | 估算的 TBSCAN Skip+Limit 的扫描比率 |
| LimitRate      | double | 估算的 TBSCAN Limit 的扫描比率 |
| HitRatio       | double | 估算的 TBSCAN 匹配记录在页中的命中率 |
| StartIOCostRatio  | double | 估算的 TBSCAN Start IO Cost占总的 IO Cost的比率 |
| StartCPUCostRatio | double | 估算的 TBSCAN Start CPU Cost占总的 CPU Cost的比率 |
| MthCPUCost     | int32  | 估算的 TBSCAN 使用匹配符过滤一个记录的 CPU 代价 |
| ReadPages      | array  | 估算的 TBSCAN 需要读取的数据页个数<br>NeedEvalIO 为 false 不需要计算<br>公式为：```min( ceil( Pages * ScanRate ) / HitRatio, Pages )``` |
| ReadRecords    | array  | 估算的 TBSCAN 需要读取的记录个数<br>公式为：```min( ceil( Records * ScanRate ) / HitRatio, Records )``` |
| IOCost         | array  | 估算的 TBSCAN 的 IO 代价公式及计算过程<br>NeedEvalIO 为 false 时不需要计算<br>即各个数据页进行顺序扫描的代价总和<br>公式为 ```SeqReadIOCostUnit * ReadPages * ( PageSize / PageUnit )``` |
| CPUCost        | array  | 估算的 TBSCAN 的 CPU 代价的公式及计算过程<br>即各个记录从数据页中提取并进行匹配符过滤的代价总和<br>公式为：```ReadRecords * ( RecExtractCPUCost + MthCPUCost )``` |
| StartCost      | array  | 估算的 TBSCAN 启动代价（内部表示）<br>公式为 ```TBScanStartCost + IOCPURate * IOCost * StartIOCostRatio + CPUCost * StartCPUCostRatio``` |
| RunCost        | array  | 估算的 TBSCAN 运行代价（内部表示）<br>公式为 ```IOCPURate * IOCost * ( 1 - StartIOCostRatio ) + CPUCost * ( 1 - StartCPUCostRatio )``` |
| TotalCost      | array  | 估算的 TBSCAN 总代价（内部表示）<br>公式为 ```StartCost + RunCost``` |
| OutputRecords  | array  | 估算的 TBSCAN 输出记录个数<br>公式为 ```max( 1, ceil( Records * MthSelectivity * LimitRate ) )``` |

示例
----

```lang-json
"ScanNode": {
  "MthSelectivity": 1,
  "ScanRate": 1,
  "LimitRate": 1,
  "HitRatio": 1,
  "StartIOCostRatio": 0,
  "StartCPUCostRatio": 0,
  "MthCPUCost": 0,
  "ReadPages": [
    "min( ceil( Pages * ScanRate ) / HitRatio, Pages )",
    "min( ceil( 74 * 1 ) / 1, 74 )",
    74
  ],
  "ReadRecords": [
    "min( ceil( Records * ScanRate ) / HitRatio, Records )",
    "min( ceil( 100000 * 1 ) / 1, 100000 )",
    100000
  ],
  "IOCost": [
    "SeqReadIOCostUnit * ReadPages * ( PageSize / PageUnit )",
    "1 * 74 * ( 65536 / 4096 ) ",
    1184
  ],
  "CPUCost": [
    "ReadRecords * ( RecExtractCPUCost + MthCPUCost )",
    "100000 * ( 4 + 0 ) ",
    400000
  ],
  "StartCost": [
    "TBScanStartCost + IOCPURate * IOCost * StartIOCostRatio + CPUCost * StartCPUCostRatio",
    "0 + 2000 * 1184 * 0 + 400000 * 0",
    0
  ],
  "RunCost": [
    "IOCPURate * IOCost * ( 1 - StartIOCostRatio ) + CPUCost * ( 1 - StartCPUCostRatio )",
    "2000 * 1184 * ( 1 - 0 ) + 400000 * ( 1 - 0 )",
    2768000
  ],
  "TotalCost": [
    "StartCost + RunCost",
    "0 + 2768000",
    2768000
  ],
  "OutputRecords": [
    "max( 1, ceil( Records * MthSelectivity * LimitRate ) )",
    "max( 1, ceil( 100000 * 1 * 1 ) )",
    100000
  ]
}
```

