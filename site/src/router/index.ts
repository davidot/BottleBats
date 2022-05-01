import { createRouter, createWebHistory } from "vue-router";
import Leaderboard from "@/views/vijf/Leaderboard.vue";
import AccountView from "@/views/AccountView.vue";
import BotView from "@/views/vijf/BotView.vue";
import ExplainView from "@/views/vijf/ExplainView.vue";
import GameView from "@/views/vijf/GameView.vue";
import VijfView from "@/views/vijf/VijfView.vue";
import ElevatedBotView from "@/views/elevated/BotView.vue";
import ElevatedExplainView from "@/views/elevated/ExplainView.vue";
import ElevatedLeaderboardView from "@/views/elevated/LeaderboardView.vue";

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes: [
    {
      path: "/",
      name: "home",
      component: ElevatedLeaderboardView,
    },
    {
      path: "/login",
      name: "login",
      component: AccountView,
    },
    {
      path: "/rules",
      name: "rules",
      component: ElevatedExplainView,
    },
    {
      path: "/bots",
      name: "bots",
      component: ElevatedBotView,
    },
    {
      path: "/vijf",
      name: "vijf",
      component: VijfView,
      children: [
        { path: "", component: Leaderboard },
        { path: "games", component: GameView },
        { path: "rules", component: ExplainView },
        { path: "bots", component: BotView },
      ],
    },
  ],
});

export default router;
