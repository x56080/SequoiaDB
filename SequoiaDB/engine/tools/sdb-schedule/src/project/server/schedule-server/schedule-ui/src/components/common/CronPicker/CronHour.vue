<template>
  <div>
    <div>
      每
      <el-select id="input_cronPicker_perHour" v-model="perHour" size="mini" style="width: 65px" @change="emitChange()">
        <el-option
          v-for="item in hourOptions"
          :key="item"
          :label="item"
          :value="item"
        />
      </el-select>
      小时运行一次
    </div>
  </div>
</template>

<script>

export default {
  name: 'CronHour',
  data() {
    return {
      hourBegin: 0,
      hourEnd: 23,
      perHour: 1,
      minute: 0
    }
  },
  computed: {
    hourOptions() {
      return [1,2,3,4,6,8,12]
    },
    
    cronExp() {
      return `0 0 */${this.perHour} * * ?`
    }
  },
  methods: {
    init(value) {
      const tempArr = value.split(' ')
      this.minute = Number(tempArr[1])
      const hourArr = tempArr[2].split('/')
      this.perHour = Number(hourArr[1])
      const hourArr2 = hourArr[0].split('-')
      this.hourBegin = Number(hourArr2[0])
      this.hourEnd = Number(hourArr2[1])
    },
    emitChange() {
      this.$emit('change', this.cronExp)
    }
  }
}
</script>

<style scoped>
.divider{
  color: #949AA6;
}
</style>
