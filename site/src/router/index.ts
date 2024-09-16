import { createRouter, createWebHistory } from "vue-router";
import AccountView from "@/views/AccountView.vue";
import BotView from "@/views/vijf/BotView.vue";
import ExplainView from "@/views/vijf/ExplainView.vue";
import LeaderboardAll from "@/views/leaderboard/Testing.vue";
import InteractiveGame from "@/views/InteractiveGame.vue";

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes: [
    {
      path: "",
      name: "home",
      component: AccountView,
      alias: ['/']
    },
    {
      path: "/login",
      name: "login",
      component: AccountView,
    },
    {
      path: "/:game",
      name: "leaderboard",
      alias: ["/:game/", "/:game/leaderboard"],
      component: LeaderboardAll,
    },
    {
      path: "/:game/play",
      name: "play",
      component: InteractiveGame,
    },
    {
      path: "/:game/rules",
      name: "rules",
      component: ExplainView,
    },
    {
      path: "/:game/my-bots",
      name: "bots",
      component: BotView,
    },
    {
      path: "/:game/admin",
      name: "admin",
      component: LeaderboardAll,
    },
  ],
});

export default router;

export const defaultGame = 'ttt';