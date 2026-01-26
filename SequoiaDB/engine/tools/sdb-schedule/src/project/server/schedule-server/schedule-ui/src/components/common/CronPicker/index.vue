<template>
  <div>
    <div >
      <el-select id="input_cronPicker_interval" v-model="type" size="small" style="width:100%" placeholder="请选择调度周期"  @change="onTypeChange">
        <el-option
          v-for="item in typeOptions"
          :key="item.value"
          :label="item.label"
          :value="item.value"
        />
      </el-select>
    </div>
    <component v-if="type" :is="currentComponent" ref="picker" @change="onChange" />
  </div>
</template>

<script>
import CronMinute from './CronMinute.vue'
import CronHour from './CronHour.vue'
import CronDay from './CronDay.vue'
import CronWeek from './CronWeek.vue'
import CronMonth from './CronMonth.vue'

export default {
  name: 'CronPicker',
  components: {
    CronMinute,
    CronHour,
    CronDay,
    CronWeek,
    CronMonth
  },
  props: {
    // Cron 表达式
    cron: {
      type: String,
      default: ''
    }
  },
  data() {
    return {
      type: '',
      typeOptions: [
        {
          value: 'minute',
          label: '分钟'
        },
        {
          value: 'hour',
          label: '小时'
        },
        {
          value: 'day',
          label: '天'
        },
        {
          value: 'week',
          label: '周'
        },
        {
          value: 'month',
          label: '月'
        }
      ]
    }
  },
  computed: {
    currentComponent() {
      return 'cron-' + this.type
    }
  },
  watch: {
    cron: {
      handler: function(newVal, oldVal) {
        if (newVal) {
          this.$nextTick(() => {
            if (this.$refs.picker) {
              this.$refs.picker.init(newVal)
            }
          })
        }
      },
      immediate: true
    }
  },
  methods: {
    onTypeChange(type) {
      this.$nextTick(() => {
        const cron = this.$refs.picker.cronExp
        this.$emit('change', {
          cron
        })
      })
    },
    reset() {
      this.type = ''
    },
    onChange(cron) {
      this.$emit('change', {
        cron
      })
    }
  }
}
</script>

<style scoped>

</style>
