<template>
  <div class="leaderboard-holder">
    <div v-if="connectionLost" style="color: red; position: relative; top: 0; width: 100%; text-align: center;">
      Connection to server lost!
    </div>
    <span v-if="results === null">Loading...</span>
    <span v-else-if="!results.cases || Object.keys(results.bots).length === 0">No data</span>
    <table v-else>
      <thead class="case-names">
        <tr>
          <td style="max-width: 250px; min-width: 250px">
            <div>
              <select v-model="stat">
                <option selected value="avg-wait">Average wait time</option>
                <option value="avg-travel">Average travel time</option>
                <option value="power">Power usage</option>
                <option value="max-wait">Max wait time</option>
                <option value="max-travel">Max travel time</option>
                <option value="total-time">Total simulation time</option>
              </select>
            </div>
          </td>
          <td v-for="cs in cases" :key="'case-' + cs.id" class="case-name">
            {{cs.name}}
          </td>
          <td style="width: 70%"></td>
          <td class="case-name">
            Worst result
          </td>
          <td class="case-name">
            Average result
          </td>
        </tr>
      </thead>
      <transition-group name="list" tag="tbody">
        <tr v-for="b in bots" :key="'bot-' + b.id">
          <td class="bot-name">
            {{ b.name }} (by {{ b.author }})
          </td>
          <td v-for="cs in cases" :key="b.id + '-' + cs.id" class="table-result" style="overflow: hidden; max-width: 35px; max-height: 35px;">
            <div v-if="b.runs[cs.id] == null" title="Not run yet" style="min-width: 35px; min-height: 35px;">

            </div>
            <div v-else-if="b.runs[cs.id].status === 'running'" style="min-height: 35px; display: flex; align-items: center">
              <spinner style="width: 50%; height: 50%; margin: auto"/>
            </div>
            <div v-else-if="b.runs[cs.id].status === 'rejected'" class="skipped" :title="b.runs[cs.id].reason|| 'Rejected'">
            </div>
            <div v-else-if="b.runs[cs.id].status !== 'done'" class="failed" :title="b.runs[cs.id].reason || 'Failed'">
              &#10060;
            </div>
            <div v-else style="min-height: 35px; display: flex; align-items: center; justify-content: center;"
                 :title="b.runs[cs.id].result[stat] + ' (' + Math.round(percentage(b.runs[cs.id].result[stat], cs.best[stat]) * 100.0) + '%)'"
                 :style="{'background-color': toColor(percentage(b.runs[cs.id].result[stat], cs.best[stat]))}">
              <PoopOrCrown :value="b.runs[cs.id].result[stat]" :limits="cs.best[stat]" />
              <!--            {{ Math.round(percentage(b.runs[cs.id].result[stat], cs.best[stat]) * 100.0) }}%-->
            </div>
          </td>
          <td></td>
          <td v-for="summ in ['worst', 'avg']" :key="b.id + '-' + summ" class="table-result" style="overflow: hidden; max-width: 35px; max-height: 35px;">
            <div v-if="b.summary == null" title="Not run yet" style="min-width: 35px; min-height: 35px;">
            </div>
            <div v-else style="min-height: 35px; max-height: 35px; display: flex; align-items: center; justify-content: center;"
                 :title="b.summary[summ].text"
                 :style="{'background-color': toColor(b.summary[summ].percentage)}">
              <PoopOrCrown :value="b.summary[summ].percentage" :limits="summaryLimits[summ]" />
            </div>
          </td>
        </tr>
      </transition-group>
<!--      <tr>-->
<!--        <td class="bot-name">-->
<!--          Best per case-->
<!--        </td>-->
<!--        <td v-for="cs in cases" :key="'summ-' + cs.id" class="table-result">-->
<!--          <div v-if="cs.best && cs.best[stat] != null" :title="cs.best[stat][0]" style="min-height: 35px;" :style="{'background-color': toColor(cs.best[stat][0], cs.best[stat])}">-->
<!--&lt;!&ndash;            {{ Math.round(percentage(cs.best[stat][0], cs.best[stat]) * 100.0) }}%&ndash;&gt;-->
<!--          </div>-->
<!--        </td>-->
<!--        <td title="Invalid"><div class="skipped"></div></td>-->
<!--        <td title="Invalid"><div class="skipped"></div></td>-->
<!--      </tr>-->
    </table>
  </div>
</template>

<script>
import Spinner from "@/components/Spinner.vue";
import {endpoint} from "@/http";
import * as raw_data from "./data.json";
import PoopOrCrown from "@/components/elevated/PoopOrCrown.vue";

export default {
  name: "LeaderboardView",
  components: {PoopOrCrown, Spinner},
  unmounted() {
    if (this.dataInterval)
      clearInterval(this.dataInterval);
  },
  mounted() {
    // this.dataInterval = setInterval(() => this.getData(), 1000);
    // this.getData();

    setTimeout(() => {
      this.results = raw_data;
    }, 1500);

  },
  data() {
    return {
      results: null,
      stat: "total-time",
      connectionLost: false,
    };
  },
  computed: {
    highBetterStat() {
      return this.stat in {};
    },
    caseNames() {
      if (!this.results?.cases)
        return {};

      const names = {};
      this.results.cases.forEach(cs => {
        names[cs.id] = cs.name;
      });
      return names;
    },
    cases() {
      if (!this.results?.cases)
        return [];

      return Object.fromEntries(
          this.results.cases
          .map(c => {
            const runs = Object.values(this.results.bots || {})
                .map(r => r.runs)
                .filter(r => r != null && r[c.id] != null)
                .map(r => r[c.id].result)
                .filter(r => r != null);
            if (runs.length === 0) {
              console.log('no runs for me', c.id);
              return c;
            }

            let [best, worst] = runs
              .map(r => r[this.stat])
              .filter(res => res != null)
              .reduce((prev, val) => {
                if (prev[0] == null)
                  return [val, val];

                if (val < prev[0])
                  return [val, prev[1]];
                if (val > prev[1])
                  return [prev[0], val];
                return prev;
              },
              [null, null]
            );

            if (this.highBetterStat) {
              const temp = worst;
              worst = best;
              best = temp;
            }

            return Object.assign({best: {[this.stat]: [best, worst]}}, c);
          })
          .sort((c1, c2) => c1.id - c2.id)
          .map(c => [c.id, c]));
    },
    bots() {
      if (!this.results?.bots)
        return [];

      return Object.entries(this.results.bots)
        .map(([id, val]) => {
          console.log('id', id, 'val', val);
          const runs = Object.entries(val?.runs || {})
              .filter(([id, r]) => r != null && r.result != null);
          if (runs.length === 0)
            return Object.assign({runs: {}}, {id: id, ...val})

          const summary = {};

          let worst = null;
          let worstId = null;
          let sum = 0;

          const lowStat = !this.highBetterStat;
          // const lowStat = !this.cases()[];


          for (const [id, run] of runs) {
            const extremes = this.cases[id]?.best?.[this.stat];
            if (extremes == null)
              continue;
            const value = this.percentage(run.result[this.stat], extremes);

            if (worst === null || (lowStat ? value < worst : value > worst)) {
              worstId = id;
              worst = value;
            }
            sum += value;
          }

          sum /= runs.length;

          summary['worst'] = { percentage: worst, text: `${Math.round(worst * 100)}% (For case: ${this.cases[worstId].name}`};
          summary['avg'] = { percentage: sum, text: `On average ${Math.round(sum * 100)}%` };

          // runs.forEach(r => console.log('d', r));




          return Object.assign({ summary, runs: {} }, { id: id, ...val })
        })
        .sort((lhs, rhs) => {
          const leftKeys = Object.keys(lhs.runs);
          const rightKeys = Object.keys(rhs.runs);
          if (leftKeys.length !== rightKeys.length)
            return rightKeys.length - leftKeys.length;

          const leftDone = leftKeys.filter(r => r.done).length;
          const rightDone = rightKeys.filter(r => r.done).length;

          console.log(leftDone, rightDone);

          if (leftDone !== rightDone)
            return leftDone - rightDone;

          return lhs.name.localeCompare(rhs.name);
        });
    },
    summaryLimits() {
      const vals = this.bots
          .map(b => b.summary)
          .filter(summ => summ != null);

      const result = { 'worst': [1, 0] };

      if (vals.length === 0)
        return result;

      let [min, max] = vals
          .map(r => r?.['avg']?.percentage)
          .filter(res => res != null)
          .reduce(([min, max], val) => {
                if (min == null)
                  return [val, val];

                if (val < min)
                  return [val, max];
                if (val > max)
                  return [min, val];
                return [min, max];
              },
              [null, null]
          );

      result['avg'] = [max, min];

      return result;
    },
  },
  methods: {
    percentage(val, [best, worst]) {
      if (best === worst) {
        if (val !== best)
          console.log('Strange??');
        return 1.0;
      }

      if (best < worst)
        return 1.0 - (val - best) / (worst - best);

      return (val - worst) / (best - worst);
    },
    toColor(p) {
      if (p === 1.0)
        return 'rgb(54, 199, 149)';

      const worstColor = [230, 6, 45];
      const bestColor = [53, 200, 69];

      // const p = this.percentage(val, [best, worst]);

      return 'rgb(' + (worstColor[0] * (1 - p) + bestColor[0] * p) + ','
                    + (worstColor[1] * (1 - p) + bestColor[1] * p) + ','
                    + (worstColor[2] * (1 - p) + bestColor[2] * p) + ')';
    },
    async getData() {
      try {
        const data = await endpoint.get("/elevated/leaderboard", {timeout: 750});
        this.results = data.data;
        this.connectionLost = false;
      } catch {
        this.connectionLost = true;
      }
    },
  },
};
</script>

<style scoped>
.case-name {
  writing-mode: vertical-lr;
  min-width: 35px;
  max-width: 35px;
  padding-bottom: 10px;

  text-align: center;
  vertical-align: middle;
}

.case-names {
  padding-left: calc(max(250px, 10%));
}

.table-result {
  width: 35px;
  max-width: 35px;
  overflow: hidden;
}

table {
  /*width: 100%;*/
  border-collapse: collapse;
  table-layout: fixed;
  border-right: 1px solid black;
  /*background: linear-gradient(180deg, rgb(54, 199, 149) 0%, !*rgb(54, 199, 149) 5%,*! rgb(53, 200, 69) 6%, rgb(230, 6, 45) 100%);*/
}

table tr {
  border-bottom: 1px solid black;
  height: 35px;
}

table td {
  border-right: 1px solid black;
  padding: 0;
}

.bot-name {
  padding-right: 5px;
  text-overflow: ellipsis;
  white-space: nowrap;
  max-width: 250px;
  overflow: hidden;
  max-height: 35px;
  min-height: 35px;
  display: table-cell;
}

.skipped {
  background:
      linear-gradient(to top left,
    rgba(0, 0, 0, 0) 0%,
    rgba(0, 0, 0, 0) calc(50% - 1.8px),
    rgba(0, 0, 0, 1) 50%,
    rgba(0, 0, 0, 0) calc(50% + 1.8px),
    rgba(0, 0, 0, 0) 100%
  );
  min-height: 35px;
  min-width: 35px;
}

.failed {
  min-height: 35px;
  max-height: 35px;
  min-width: 35px;
  font-size: 30px;
  display: flex;
  flex-direction: row;
  justify-content: center;
  text-align: center;
}

.list-move,
.list-enter-active,
.list-leave-active {
  transition: transform 0.5s ease 0s;
}
.list-enter-from,
.list-leave-to {
  opacity: 0;
  transform: translateY(30px);
}

.list-leave-active {
  position: absolute;
}

.leaderboard-holder {
  padding-right: 30px;
  padding-left: 10px;
}
</style>
