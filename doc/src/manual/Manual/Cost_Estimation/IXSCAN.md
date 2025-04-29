
IXSCAN 的推演公式将展示以下信息：

| 字段名            | 类型   | 描述 |
| ----------------- | ------ | ---- |
| IndexPages        | int32  | 估算的 IXSCAN 输入的索引页数 |
| IndexLevels       | int32  | 估算的 IXSCAN 输入的索引层数 |
| MthSelectivity    | int32  | 估算的 IXSCAN 使用匹配符进行过滤的选择率 |
| ScanRate          | double | 估算的 IXSCAN Skip+Limit 的扫描比率 |
| LimitRate         | double | 估算的 IXSCAN Limit 的扫描比率 |
| MthCPUCost        | int32  | 估算的 IXSCAN 使用匹配符过滤一个记录的 CPU 代价 |
| IXScanSelectivity | double | 估算的 IXSCAN 使用索引时需要扫描索引的比例 |
| IXPredSelectivity | double | 估算的 IXSCAN 使用索引进行过滤的选择率 |
| StartIOCostRatio  | double | 估算的 IXSCAN Start IO Cost占总的 IO Cost的比率 |
| StartCPUCostRatio | double | 估算的 IXSCAN Start CPU Cost占总的 CPU Cost的比率 |
| PredCPUCost       | int32  | 估算的 IXSCAN 使用索引进行过滤一个记录的 CPU 代价 |
| IndexReadPages    | array  | 估算的 IXSCAN 需要读取的索引页个数<br>NeedEvalIO 为 false 不需要计算<br>公式为：```max( 1, ceil( IndexPages * IXScanSelectivity * ScanRate ) )``` |
| IndexReadRecords  | array  | 估算的 IXSCAN 需要读取的索引记录个数<br>公式为：```max( 1, ceil( Records * IXScanSelectivity * ScanRate ) )``` |
| ReadPages         | array  | 估算的 IXSCAN 需要读取的数据页个数<br>NeedEvalIO 为 false 不需要计算<br>公式为：```max( 1, ceil( Pages * IXPredSelectivity * ScanRate ) )``` |
| ReadRecords       | array  | 估算的 IXSCAN 需要读取的记录个数<br>公式为：```max( 1, ceil( Records * IXPredSelectivity * ScanRate ) )``` |
| IOCost            | array  | 估算的 IXSCAN 的 IO 代价的公式及计算过程<br>NeedEvalIO 为 false 不需要计算<br>即各个数据页进行随机扫描的代价总和<br>公式为：```RandomReadIOCostUnit * ( IndexReadPages + ReadPages ) * ( PageSize / PageUnit )``` |
| CPUCost           | array  | 估算的 IXSCAN 的 CPU 代价的公式及计算过程<br>即各个记录从索引页和数据页中提取并进行匹配符过滤的代价总和<br>如果需要进行匹配符过滤，公式为：```IndexReadRecords * ( IXExtractCPUCost + PredCPUCost ) + ReadRecords * ( RecExtractCPUCost + MthCPUCost )```<br>如果不需要进行匹配符过滤，公式为：```IndexReadRecords * ( IXExtractCPUCost + PredCPUCost ) + ReadRecords * RecExtractCPUCost``` |
| StartCost         | array  | 估算的 IXSCAN 的启动代价（内部表示）<br>公式为：```IXScanStartCost + PredCPUCost * IndexLevels + IOCPURate * IOCost * StartIOCostRatio + CPUCost * StartCPUCostRatio``` |
| RunCost           | array  | 估算的 IXSCAN 的运行代价（内部表示）<br>公式为：```IOCPURate * IOCost * ( 1 - StartIOCostRatio ) + CPUCost * ( 1 - StartCPUCostRatio )``` |
| TotalCost         | array  | 估算的 IXSCAN 的总代价（内部表示）<br>公式为：```StartCost + RunCost``` |
| OutputRecords     | array  | 估算的 IXSCAN 的输出记录个数<br>公式为：```max( 1, ceil( Records * min( IXPredSelectivity, MthSelectivity ) * LimitRate ) )``` |

示例
----

```lang-json
"ScanNode": {
  "IndexPages": 49,
  "IndexLevels": 1,
  "MthSelectivity": 0.00001,
  "ScanRate": 1,
  "LimitRate": 1,
  "MthCPUCost": 2,
  "IXScanSelectivity": 0.00001,
  "IXPredSelectivity": 0.00001,
  "StartIOCostRatio": 0,
  "StartCPUCostRatio": 0,
  "PredCPUCost": 1,
  "IndexReadPages": [
    "max( 1, ceil( IndexPages * IXScanSelectivity * ScanRate ) )",
    "max( 1, ceil( 49 * 1e-05 * 1 ) )",
    1
  ],
  "IndexReadRecords": [
    "max( 1, ceil( Records * IXScanSelectivity * ScanRate ) )",
    "max( 1, ceil( 100000 * 1e-05 * 1 ) )",
    1
  ],
  "ReadPages": [
    "max( 1, ceil( Pages * IXPredSelectivity * ScanRate ) )",
    "max( 1, ceil( 49 * 1e-05 * 1 ) )",
    1
  ],
  "ReadRecords": [
    "max( 1, ceil( Records * IXPredSelectivity * ScanRate ) )",
    "max( 1, ceil( 100000 * 1e-05 * 1 ) )",
    1
  ],
  "IOCost": [
    "RandomReadIOCostUnit * ( IndexReadPages + ReadPages ) * ( PageSize / PageUnit )",
    "10 * ( 1 + 1 ) * ( 65536 / 4096 ) ",
    320
  ],
  "CPUCost": [
    "IndexReadRecords * ( IXExtractCPUCost + PredCPUCost ) + ReadRecords * RecExtractCPUCost",
    "1 * ( 2 + 1 ) + 1 * 4",
    7
  ],
  "StartCost": [
    "IXScanStartCost + PredCPUCost * IndexLevels + IOCPURate * IOCost * StartIOCostRatio + CPUCost * StartCPUCostRatio",
    "0 + 1 * 1 + 2000 * 320 * 0 + 7 * 0",
    1
  ],
  "RunCost": [
    "IOCPURate * IOCost * ( 1 - StartIOCostRatio ) + CPUCost * ( 1 - StartCPUCostRatio )",
    "2000 * 320 * ( 1 - 0 ) + 7 * ( 1 - 0 )",
    640007
  ],
  "TotalCost": [
    "StartCost + RunCost",
    "1 + 640007",
    640008
  ],
  "OutputRecords": [
    "max( 1, ceil( Records * min( IXPredSelectivity, MthSelectivity ) * LimitRate ) )",
    "max( 1, ceil( 100000 * min( 0.00001, 0.00001 ) * 1 ) )",
    1
  ]
}
```
