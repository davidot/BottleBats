<template>
    <input type="checkbox" v-model="sortDesc"> Sort desc
    <div class="leaderboard" :style="{'--num-columns': Object.keys(stats).length}">
        <div></div>
        <div>Bot</div>
        <div v-for="(value, stat) in stats">
            Stat {{ stat }}: {{ value }}
        </div>
        <TransitionGroup name="leaderboard-bots">
        <template v-for="bot in botsView" :key="'bot-' + bot.id">
            <img src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAADIAAAAyCAYAAAAeP4ixAAADfElEQVRoQ+2Xe2hPYRjHP7u5ZbFMGbkmcmtFLkVNueR+vyyUS4hoxsbIxhLmzpAhMrlERoQR/hh/LEVbuTXlVqQ2hmmLbbZ53p2tdz9+jc55T43O8995z/s+5/t9vs95n+cJ4OqbKv4DC/CINDAVPUUamCB4iniKuBQBL7VcCqxtt54itkPn0kFPkerAntkD5/brGPeNguR0CAj4c9xvnIFD6/W+Lr1g1yUIafzns352OFOkogISZ8OT+9r13ASYtrR+MG/yIG4SlH239jVtDvuuQtvOtkioQ86IKA+F+bBiDBQVWiCCgiHlPPTo5x9U6TdYOR7evtDv4/bBUCHmwJwTUR9/mAWb5kNVzWjTuh3sz4TmLX6Hlroa7lzQ6yNmQMwOBxSso2aIKE8nUuDSEQ1o4EhJu6O+ALMuw+5Yvda+K+yVlGrctAERqfgBayW6eTka1KINMGGB9fz+NcSOg28l1nOjJrDnCnTs7piEWUWUt4J38r+MheIiC1xwI9h5UcB2g/gp8OqpBr1sK4yaZYSEeSLK4/1bsGWxBhjREXoPgttyAdTaEFEm4aAxEu4QUV6PJMO1dP9A23SA1OvQLPQfIFJeBmumwovHvmCDQ2B7BnSLNErCPUWU55y7sHGuL+DoGJi9yjgJ94iUl0rlngyvn/mCNnjd/hoNc3Wkrue0JMg85T/yw6fLzbbTuCrmiWTflBZlSZ1bq5O0LUHw7qVei0+FqIlGyZgloupIjPRdJV8tkLV1RPVf8apJlJRTVt0kXpMmUUgaMnNEVGVPkLR5nquh1a3smachLVG/69oHdkixDJGiacDMEUnfBhcPa0gDhkPSMV+IKdLeZ9/QaxOlfVkobYwBM0NEXbXJ83T3Gx4h3a8ADm3pC7FYUi5WUi9fUrDWko7DgGGOqTgn8qnAmke+fLTABMqPnXIOevb3Dy5PUm+tpKBKRWWhYXBASLdq44iMMyKVlZI+c+BRtgYxJx5mLq8fVEYanNyu9/QeCJvPWrebTXNGRM3ram6vtcjBMmBJ/QgMrB+OGsBU1c+9p/dFr5Cqv9ImDSeD1bMHsC4aKmVuV9Yy3Povwlr/HRiVijGj4fOHmpQU8kqVPtIp2zBnitj4oFtHPCJuRdauX08Ru5Fz65yniFuRtevXU8Ru5Nw65yniVmTt+vUUsRs5t855irgVWbt+fwLEx0DGsBW5nQAAAABJRU5ErkJggg==">
            <div>{{ bot.name }}</div>
            <div v-for="(value, stat) in stats">
                {{ stat }}
            </div>
        </template>
        </TransitionGroup>
    </div>
</template>


<script setup>
import BotItem from "@/components/vijf/BotItem.vue";
import { noCasesData, withCasesData } from "@/views/leaderboard/data.js";
import { computed, ref, reactive } from 'vue';

console.log(noCasesData);

const withCases = ref(false);

const data = reactive(noCasesData);

const stats = computed(() => {
    return data.allStats;
});

function statInfo(stat) {
    return data.allStats[stat];
}

const bots = computed(() => {
    return data.bots;
});

const sortDesc = ref(false);

const botsView = computed(() => {
    const filteredBots = bots.value.filter(() => true);
    console.log(filteredBots);
    filteredBots.sort((a, b) => a.name.localeCompare(b.name) * (sortDesc.value ? -1 : 1));
    return filteredBots;
});


</script>

<style scoped>
.leaderboard {
    display: grid;
    grid-template-columns: 50px 2fr repeat(var(--num-columns), 1fr);
}

.leaderboard-bots-move {
    transition: all 0.5s ease;
}
</style>