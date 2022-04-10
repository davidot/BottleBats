import { createRouter, createWebHistory } from "vue-router";
import Leaderboard from "@/components/Leaderboard.vue";
import AccountView from "@/views/AccountView.vue";

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
      path: "/login",
      name: "bots",
      component: AccountView,
    },
    {
      path: "/login",
      name: "add-bot",
      component: AccountView,
    },
    {
      path: "/about",
      name: "about",
      // route level code-splitting
      // this generates a separate chunk (About.[hash].js) for this route
      // which is lazy-loaded when the route is visited.
      component: () => import("../views/AboutView.vue"),
    },
  ],
});

export default router;
