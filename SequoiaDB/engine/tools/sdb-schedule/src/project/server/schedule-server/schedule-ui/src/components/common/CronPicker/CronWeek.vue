<template>
  <div>
    <div>
      <div>
        每周
        <el-select id="input_cronPicker_weeks" v-model="weeks" multiple size="mini" style="width: 200px" @change="emitChange()">
          <el-option
            v-for="item in weekOptions"
            :key="item.value"
            :label="item.label"
            :value="item.value"
          />
        </el-select>
        运行一次
      </div>
    </div>
  </div>
</template>

<script>

export default {
  name: 'CronWeek',
  data() {
    return {
      weeks: [],
    }
  },
  computed: {
    weekOptions() {
      return [{
        value: 'MON',
        label: '周一'
      }, {
        value: 'TUE',
        label: '周二'
      }, {
        value: 'WED',
        label: '周三'
      }, {
        value: 'THU',
        label: '周四'
      }, {
        value: 'FRI',
        label: '周五'
      }, {
        value: 'SAT',
        label: '周六'
      }, {
        value: 'SUN',
        label: '周日'
      }]
    },
   
    cronExp() {
      if (this.weeks.length === 0) {
        return `0 0 0 ? * MON`
      }
      return `0 0 0 ? * ${this.weeks.join(',')}`
    }
  },
  methods: {
    init(value) {
      const tempArr = value.split(' ')
      if(tempArr[5] === '?'){
        this.weeks = []
      }else{
        const weekArr = tempArr[5].split(',')
        this.weeks = weekArr.filter(v => v !== '').map(v => v)
      }

    },
    emitChange() {
      this.$emit('change', this.cronExp)
    }
  }
}
</script>

<style scoped>

</style>
