<template>
    <div class="leaderboard" :style="{'--num-columns': Object.keys(stats).length}">
        <div></div>
        <div @click="sortOnCol('name', false)" :class="['stat-header', sortOn === 'name' ? sortDirection : '']">Bot name (creator)</div>
        <div v-for="(value, stat) in stats" :class="['stat-header', sortOn === stat ? sortDirection : '']" @click="sortOnCol(stat)">
            {{ stat }}
        </div>
        <TransitionGroup name="leaderboard-bots">
        <template v-for="bot in botsView" :key="'bot-' + bot.id">
            <img src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAADIAAAAyCAYAAAAeP4ixAAADfElEQVRoQ+2Xe2hPYRjHP7u5ZbFMGbkmcmtFLkVNueR+vyyUS4hoxsbIxhLmzpAhMrlERoQR/hh/LEVbuTXlVqQ2hmmLbbZ53p2tdz9+jc55T43O8995z/s+5/t9vs95n+cJ4OqbKv4DC/CINDAVPUUamCB4iniKuBQBL7VcCqxtt54itkPn0kFPkerAntkD5/brGPeNguR0CAj4c9xvnIFD6/W+Lr1g1yUIafzns352OFOkogISZ8OT+9r13ASYtrR+MG/yIG4SlH239jVtDvuuQtvOtkioQ86IKA+F+bBiDBQVWiCCgiHlPPTo5x9U6TdYOR7evtDv4/bBUCHmwJwTUR9/mAWb5kNVzWjTuh3sz4TmLX6Hlroa7lzQ6yNmQMwOBxSso2aIKE8nUuDSEQ1o4EhJu6O+ALMuw+5Yvda+K+yVlGrctAERqfgBayW6eTka1KINMGGB9fz+NcSOg28l1nOjJrDnCnTs7piEWUWUt4J38r+MheIiC1xwI9h5UcB2g/gp8OqpBr1sK4yaZYSEeSLK4/1bsGWxBhjREXoPgttyAdTaEFEm4aAxEu4QUV6PJMO1dP9A23SA1OvQLPQfIFJeBmumwovHvmCDQ2B7BnSLNErCPUWU55y7sHGuL+DoGJi9yjgJ94iUl0rlngyvn/mCNnjd/hoNc3Wkrue0JMg85T/yw6fLzbbTuCrmiWTflBZlSZ1bq5O0LUHw7qVei0+FqIlGyZgloupIjPRdJV8tkLV1RPVf8apJlJRTVt0kXpMmUUgaMnNEVGVPkLR5nquh1a3smachLVG/69oHdkixDJGiacDMEUnfBhcPa0gDhkPSMV+IKdLeZ9/QaxOlfVkobYwBM0NEXbXJ83T3Gx4h3a8ADm3pC7FYUi5WUi9fUrDWko7DgGGOqTgn8qnAmke+fLTABMqPnXIOevb3Dy5PUm+tpKBKRWWhYXBASLdq44iMMyKVlZI+c+BRtgYxJx5mLq8fVEYanNyu9/QeCJvPWrebTXNGRM3ram6vtcjBMmBJ/QgMrB+OGsBU1c+9p/dFr5Cqv9ImDSeD1bMHsC4aKmVuV9Yy3Povwlr/HRiVijGj4fOHmpQU8kqVPtIp2zBnitj4oFtHPCJuRdauX08Ru5Fz65yniFuRtevXU8Ru5Nw65yniVmTt+vUUsRs5t855irgVWbt+fwLEx0DGsBW5nQAAAABJRU5ErkJggg==">
            <div>{{ bot.name }}</div>
            <div v-for="(value, stat) in stats">
                {{ bot.stats[stat] }}
            </div>
        </template>
        </TransitionGroup>
    </div>
</template>


<script setup>
import BotItem from "@/components/vijf/BotItem.vue";
import { noCasesData, noCasesData2 } from "@/views/leaderboard/data.js";
import { computed, ref, reactive, onMounted } from 'vue';

console.log(noCasesData);

const withCases = ref(false);
const sortDirection = ref('asc');

// Get default sort from data!!!!
const sortOn = ref(null);

const data = ref(noCasesData);

const stats = computed(() => {
    return data.value.allStats;
});

const bots = computed(() => {
    return data.value.bots;
});

const botsView = computed(() => {
    const filteredBots = bots.value.filter(() => true);
    const sortCol = sortOn.value;
    const sortAsc = sortDirection.value === 'asc' || sortCol == null;
    if (sortCol === 'name' || sortCol == null) {
        filteredBots.sort((a, b) => a.name.localeCompare(b.name) * (sortAsc ? 1 : -1));
    } else {
        filteredBots.sort((a, b) => (a.stats[sortCol] - b.stats[sortCol]) * (sortAsc ? 1 : -1));
    }
    return filteredBots;
});

function sortOnCol(newCol, checkPref=true) {
    let sortPreference = 'asc';
    if (checkPref) {
        const statPreference = stats.value[newCol];
        if ('sortPreference' in statPreference) {
            sortPreference = statPreference.sortPreference;
        } else if ('higherIntent' in statPreference && statPreference.higherIntent === 'positive') {
            sortPreference = 'desc';
        }
    }

    if (newCol != sortOn.value) {
        sortOn.value = newCol;
        sortDirection.value = sortPreference;
    } else {
        if (sortDirection.value === sortPreference) {
            if (sortDirection.value === 'desc') {
                sortDirection.value = 'asc';
            } else {
                sortDirection.value = 'desc';
            }
        } else {
            sortOn.value = null;
        }
    }

}

onMounted(() => {
    setTimeout(() => {
        data.value = noCasesData2;
    }, 5000);
})

</script>

<style scoped>
.leaderboard {
    display: grid;
    grid-template-columns: 50px 2fr repeat(var(--num-columns), 1fr);
    gap: 0.5em 1em;
}

.leaderboard-bots-move,
.leaderboard-bots-enter-active,
.leaderboard-bots-leave-active {
  transition: all 0.5s ease;
}

.leaderboard-bots-enter-from {
  opacity: 0;
  transform: translateX(30px);
}

.stat-header {
    position: relative;
    min-height: 2em;
    padding: 2px;
}

.stat-header.desc::after,
.stat-header.asc::after {
    content: '';
    border: solid black;
    border-width: 0 2px 2px 0;
    width: 1em;
    height: 1em;
    position: absolute;
    right: 10px;
    top: 0;
}

.stat-header.desc::after {
    transform: rotate(45deg);
}

.stat-header.asc::after {
    transform: translate(0, 50%) rotate(-135deg);
}

.stat-header.asc,
.stat-header.desc {
    background-color: gray;
}
</style>