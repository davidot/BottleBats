import { createRouter, createWebHistory } from "vue-router";
import Leaderboard from "@/components/Leaderboard.vue";
import AccountView from "@/views/AccountView.vue";
import BotView from "@/views/BotView.vue";

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes: [
    {
      path: "/",
      name: "home",
      component: Leaderboard,
    },
    {
      path: "/login",
      name: "login",
      component: AccountView,
    },
    {
      path: "/login",
      name: "games",
      component: AccountView,
    },
    {
      path: "/bots",
      name: "bots",
      component: BotView,
    },
  ],
});

export default router;
