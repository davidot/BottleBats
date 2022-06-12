<template>
  <div class="leaderboard-holder" style="position: relative">
    <div class="disconnected" :style="{top: connectionLost ? '0' : '-100%', 'flex-direction': (isRuben && connectionLost) ? 'column' : 'row'}">
      <img src="/siren1.gif" style="height: 1.5em" alt="siren which means things are going wrong"/>
      <span style="border: 1px solid red; padding: 2px 5px; font-weight: bold">
        Connection to server lost!
      </span>
      <img src="/siren1.gif" style="height: 1.5em" alt="siren which means things are going wrong"/>
    </div>
    <span v-if="results === null">Loading...</span>
    <span v-else-if="!results.cases || Object.keys(results.bots || {}).length === 0">No data</span>
    <table v-else>
      <thead class="case-names">
        <tr>
          <td style="max-width: 250px; min-width: 250px; position: relative;">
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
            <div style="position: absolute; bottom: 0" title="Click to sort on names" @click="sortOn('name')">
              Names
              <SortingIndicator property-name="name" />
            </div>
          </td>
          <td v-for="cs in cases" :key="'case-' + cs.id" class="case-name">
            {{ cs.name }}
          </td>
          <td style="width: 70%"></td>
          <td class="case-name" @click="sortOn('worst')">
            Worst result
            <SortingIndicator property-name="worst" />
          </td>
          <td class="case-name" @click="sortOn('avg')">
            Average result
            <SortingIndicator property-name="avg" />
          </td>
        </tr>
      </thead>
      <transition-group name="list" tag="tbody">
        <tr v-for="b in bots" :key="'bot-' + b.id">
          <td class="bot-name" :title="b.name" :data-bot-id="b.id">
            <img v-if="b.hasImage" :src="'/api/elevated/bot-image/' + b.id" style="width: 31px; height: 31px" alt="" loading="lazy"/>
            <span style="text-overflow: ellipsis; max-width: calc(100% - 33px); overflow-x: visible">
              {{ b.name }} (by {{ b.author }})
            </span>
          </td>
          <td v-for="cs in cases" :key="b.id + '-' + cs.id" class="table-result" style="max-width: 35px; max-height: 35px;">
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
          <td v-for="summ in ['worst', 'avg']" :key="b.id + '-' + summ" class="table-result" style="max-width: 35px; max-height: 35px;">
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
    </table>
  </div>
</template>

<script>
import Spinner from "@/components/Spinner.vue";
import {endpoint} from "@/http";
import PoopOrCrown from "@/components/elevated/PoopOrCrown.vue";
import SortingIndicator from "@/components/elevated/SortingIndicator.vue";
import {computed} from "vue";

export default {
  name: "LeaderboardView",
  components: { SortingIndicator, PoopOrCrown, Spinner },
  unmounted() {
    if (this.dataInterval)
      clearInterval(this.dataInterval);
  },
  mounted() {
    this.dataInterval = setInterval(() => this.getData(), 2000);
    this.getData();
  },
  inject: ["userDetails"],
  data() {
    return {
      results: null,
      stat: "avg-wait",
      connectionLost: false,
      sortingOn: "name",
      invertSorting: false,
    };
  },
  provide() {
    return {
      sorting: {
        on: computed(() => this.sortingOn),
        inverted: computed(() => this.invertSorting),
      },
    };
  },
  computed: {
    isRuben() {
      return this.userDetails.values.value.isRuben;
    },
    highBetterStat() {
      return this.stat in {};
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
          const runs = Object.entries(val?.runs || {})
              .filter(([id, r]) => r != null && r.result != null);
          if (runs.length === 0)
            return Object.assign({runs: {}}, {id: id, ...val})

          const summary = {};

          let worst = null;
          let worstId = null;
          let sum = 0;

          const lowStat = !this.highBetterStat;

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

          return Object.assign({ summary, runs: {} }, { id: id, ...val })
        })
        .sort((lhs, rhs) => {
          const leftKeys = Object.entries(lhs.runs);
          const rightKeys = Object.entries(rhs.runs);
          if (leftKeys.length !== rightKeys.length)
            return rightKeys.length - leftKeys.length;

          const leftDone = leftKeys.filter(([, v]) => v.status === 'done').length;
          const rightDone = rightKeys.filter(([, v]) => v.status === 'done').length;

          if (leftDone !== rightDone)
            return rightDone - leftDone;

          if ((this.sortingOn === 'worst' || this.sortingOn === 'avg') && lhs.summary && rhs.summary) {
            const lhsValue = lhs.summary[this.sortingOn].percentage;
            const rhsValue = rhs.summary[this.sortingOn].percentage;
            // Default (invert = false) means -1 since highest value is best for percentage
            if (lhsValue !== rhsValue)
              return (this.invertSorting ? 1 : -1) * (lhsValue - rhsValue);
          } else if (this.sortingOn === 'name' && this.invertSorting) {
            return rhs.name.localeCompare(lhs.name);
          }

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
    sortOn(name) {
      console.log('sortOn', name, this.sortingOn, this.invertSorting);
      if (this.sortingOn === name) {
        this.invertSorting = !this.invertSorting;
      } else {
        this.sortingOn = name;
        this.invertSorting = false;
      }
      console.log('sortOn', name, this.sortingOn, this.invertSorting);
    }
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
  /*overflow: hidden;*/
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
  display: flex;
  align-content: center;
  align-items: center;
  gap: 2px;
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
