<template>
  <div class="vijf-leaderboard-item">
    {{ data.name }} (by {{ data.user }})
    Potjes gespeeld: {{ data.played }} Gewonnen: {{ data.won }}
    Percentage: {{ Math.round(100 * data.won / data.played) }}%
    <slot />
  </div>
</template>

<script>
export default {
  name: "VijfLeaderboardItem",
  props: {
    data: {},
  },
};
</script>

<style scoped>

.vijf-leaderboard-item {

}

</style>
