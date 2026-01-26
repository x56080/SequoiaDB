<template>
  <div>
    <div>
      <div>
        每月
         <el-date-picker 
          size="mini" style="width: 200px"
          popper-class="days_picker"
          type="dates"
          v-model="days"
          :editable = "false"
          default-value="1111-01-01"
          format="dd"
          value-format="yyyy-MM-dd"
          placeholder="选择一个或多个日期"
          @change="emitChange()">
        </el-date-picker>
        日，运行一次
      </div>
    </div>
  </div>
</template>

<script>

export default {
  name: 'CronMonth',
  data() {
    return {
      days: [],
    }
  },
  computed: {
    cronExp() {
      if (!this.days || this.days.length === 0) {
        return `0 0 0 * * ?`
      }
      return `0 0 0 ${this.days.map(date => date.split("-")[2])} * ?`
    }
  },
  methods: {
    init(value) {
      const prefix = "1111-01-"
      const tempArr = value.split(' ')
      if(tempArr[3] === '*'){
        this.days = []
      }else{
        const dayArr = tempArr[3].split(',')
        this.days = dayArr.filter(v => v !== '').map(v => prefix + v)
      }
    },
    emitChange() {
      this.$emit('change', this.cronExp)
    }
  }
}
</script>

<style>

.days_picker .el-date-picker__header,
.days_picker .el-picker-panel__content .prev-month,
.days_picker .el-picker-panel__content .next-month,
.days_picker .el-picker-panel__content .el-date-table tbody tr:first-child {
  display: none !important;
}

</style>
