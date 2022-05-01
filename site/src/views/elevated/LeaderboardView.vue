<template>
  <div>
    <span v-if="results === null">Loading...</span>
    <span v-else-if="!results.cases || Object.keys(results.bots).length === 0">No data</span>
    <table v-else>
      <thead class="case-names">
        <tr>
          <td style="max-width: 250px; min-width: 250px"></td>
          <td v-for="cs in cases" :key="'case-' + cs.id" class="case-name">
            {{cs.name}}
          </td>
          <td class="case-name">
            Worst result
          </td>
          <td class="case-name">
            Average result
          </td>
        </tr>
      </thead>
      <tr v-for="b in bots" :key="'bot-' + b.id">
        <td class="bot-name">
          {{ b.name }} (by {{ b.author }})
        </td>
        <td v-for="cs in cases" :key="b.id + '-' + cs.id" class="table-result" style="overflow: hidden; max-width: 35px; max-height: 35px;">
          <div v-if="b.runs[cs.id] == null" title="Not run yet" style="min-width: 35px; min-height: 35px;">

          </div>
          <div v-else-if="!b.runs[cs.id].done" style="min-height: 35px; display: flex; align-items: center">
            <spinner style="width: 50%; height: 50%; margin: auto"/>
          </div>
          <div v-else-if="b.runs[cs.id].rejected" class="skipped">
          </div>
          <div v-else style="min-height: 35px;" :title="b.runs[cs.id].result[stat]" :style="{'background-color': toColor(b.runs[cs.id].result[stat], cs.best[stat])}">
<!--            {{ Math.round(percentage(b.runs[cs.id].result[stat], cs.best[stat]) * 100.0) }}%-->
          </div>
        </td>
        <td v-for="summ in ['worst', 'avg']" :key="b.id + '-' + summ" class="table-result" style="overflow: hidden; max-width: 35px; max-height: 35px;">
          ?
        </td>
      </tr>
      <tr>
        <td class="bot-name">
          Best per case
        </td>
        <td v-for="cs in cases" :key="'summ-' + cs.id" class="table-result">
          <div v-if="cs.best && cs.best[stat] != null" :title="cs.best[stat][0]" style="min-height: 35px;" :style="{'background-color': toColor(cs.best[stat][0], cs.best[stat])}">
<!--            {{ Math.round(percentage(cs.best[stat][0], cs.best[stat]) * 100.0) }}%-->
          </div>
        </td>
        <td title="Invalid"><div class="skipped"></div></td>
        <td title="Invalid"><div class="skipped"></div></td>
      </tr>
    </table>
  </div>
</template>

<script>
import Spinner from "@/components/Spinner.vue";

export default {
  name: "LeaderboardView",
  components: {Spinner},
  mounted() {
    setTimeout(() => {
      this.results = {
        cases: [
          { id: 0, name: "Case #0", best: { "avg-wait": [13.4, 27.3] } },
          { id: 1, name: "Case #1" },
          { id: 2, name: "Case #2" },
        ],
        bots: {
          13: {
            name: "Simple bot",
            runs: {
              0: {
                done: true,
                result: {
                  "avg-wait": 27.3,
                }
              },
              1: {
                done: false
              },
              2: {
                done: true,
                rejected: true,
              }
            }
          },
          24: {
            name: "Better bot",
            runs: {
              0: {
                done: true,
                result: {
                  "avg-wait": 13.4,
                }
              }
            }
          },
          240: {
            name: "Bot with a very very very very long name like insanely long",
            runs: {
              0: {
                done: false,
              },
              1: {
                done: false,
              },
              2: {
                done: false,
              },
            }
          }
        }
      };
    }, 0);

    setTimeout(() => {
      this.results = {
        cases: [
          { id: 0, name: "Case #0", best: { "avg-wait": [13.4, 27.3] } },
          { id: 1, name: "Case #1", best: { "avg-wait": [14.2, 14.2] } },
          { id: 2, name: "Case #2" },
        ],
        bots: {
          13: {
            name: "Simple bot",
            runs: {
              0: {
                done: true,
                result: {
                  "avg-wait": 27.3,
                }
              },
              1: {
                done: true,
                result: {
                  "avg-wait": 14.2,
                },
              },
              2: {
                done: true,
                rejected: true,
              }
            },
          },
          24: {
            name: "Better bot",
            runs: {
              0: {
                done: true,
                result: {
                  "avg-wait": 13.4,
                },
              },
            },
          },
          240: {
            name: "Bot with a very very very very long name like insanely long",
            runs: {
              0: {
                done: true,
                result: {
                  "avg-wait": 13.5,
                },
              },
              1: {
                done: false,
              },
              2: {
                done: true,
                rejected: true,
              }
            }
          },
          241: {
            name: "Bot #123",
            runs: {
              0: {
                done: true,
                result: {
                  "avg-wait": 18.5,
                },
              },
            }
          },
          242: {
            name: "Bot #1231",
            runs: {
              0: {
                done: true,
                result: {
                  "avg-wait": 20.5,
                },
              },
            }
          }
        }
      };
    }, 3000);
  },
  data() {
    return {
      results: null,
      stat: "avg-wait",
    };
  },
  computed: {
    cases() {
      if (!this.results?.cases)
        return [];

      return this.results.cases.map(c => c).sort((c1, c2) => c1.id - c2.id);
    },
    bots() {
      if (!this.results?.bots)
        return [];

      return Object.entries(this.results.bots).map(([id, val]) => {
        return Object.assign({ runs: {} }, { id: id, ...val });
      });
    }
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
    toColor(val, [best, worst]) {
      if (val === best)
        return 'rgb(54, 199, 149)';

      const worstColor = [230, 6, 45];
      const bestColor = [53, 200, 69];

      const p = this.percentage(val, [best, worst]);

      return 'rgb(' + (worstColor[0] * (1 - p) + bestColor[0] * p) + ','
                    + (worstColor[1] * (1 - p) + bestColor[1] * p) + ','
                    + (worstColor[2] * (1 - p) + bestColor[2] * p) + ')';
    },
  },
};
</script>

<style scoped>
.case-name {
  writing-mode: sideways-lr;
  min-width: 35px;
  max-width: 35px;
  padding-bottom: 10px;
}

.case-names {
  padding-left: calc(max(250px, 10%));
}

.table-result {
  width: 25px;
  max-width: 25px;
  overflow: hidden;
}

table {
  border-collapse: collapse;
  table-layout: fixed;
  background: linear-gradient(0deg, rgb(54, 199, 149) 0%, rgb(54, 199, 149) 5%, rgb(53, 200, 69) 6%, rgb(230, 6, 45) 100%);
}

table tr {
  border-bottom:1px solid black;
  height: 25px;
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
  max-height: 25px;
  min-height: 25px;
  display: table-cell;
}

.skipped {
  background:
      linear-gradient(to top left,
      rgba(0,0,0,0) 0%,
      rgba(0,0,0,0) calc(50% - 1.8px),
      rgba(0,0,0,1) 50%,
      rgba(0,0,0,0) calc(50% + 1.8px),
      rgba(0,0,0,0) 100%);
  min-height: 35px;
  min-width: 35px;
}

</style>
