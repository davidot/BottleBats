<template>
    Hello: {{ game }}
    <div v-show="!hasRunningGame">
        <h3>Setup new game of type {{ game }}</h3>

        <button v-if="!hasRunningGame" @click="startSingleplayer">Quick start single player game</button>
        <table v-if="gameSetupConfig.numPlayers !== -1 && !hasRunningGame">
            <thead>
                <th>
                    <td>
                        Players
                    </td>
                </th>
            </thead>
            <tbody>
                <tr v-for="index in gameSetupConfig.numPlayers">
                    <td>
                        <label>Player {{ index }}</label>
                        <select v-model="pickedAlgos[index - 1]">
                            <option value="" selected>Player</option>
                            <template v-for="algo in gameSetupConfig.availableAlgos">
                                <option :value="algo">{{ algo }}</option>
                            </template>
                        </select>
                    </td>
                </tr>
                <tr>
                    <td>
                        <button>Start game!</button>
                    </td>
                </tr>
            </tbody>
        </table>
    </div>
    <Console :messages="messages" :waiting-on-us="waitingOnUs" @sendMessage="pushMessage"/>
</template>

<script>
import { endpoint } from "@/http";
import Console from "./Console.vue";



export default {
    components: {
        Console,
    },
    props: {
        game: String,
    },
    mounted() {
        endpoint.get("/game-info/" + this.game)
            .then((val) => {
                console.log("Got val: " + val.data);
                this.gameSetupConfig = val.data;
                for (let i = 0; i < val.data.numPlayers; ++i) {
                    this.pickedAlgos.push("");
                }
            });
    },
    computed: {
        hasRunningGame() {
            return this.ws != null;
        }
    },
    data() {
        return {
            ws: null,
            waitingOnUs: false,
            gameSetupConfig: {
                numPlayers: -1,
                availableAlgos: [],
            },
            pickedAlgos: [],
            messages: [
                {from: "game", content: "Guessed 84 wrong!"},
                {from: "game", content: "Guessed 1234 wrong!"},
                {from: "game", content: "guess"},
                {from: "me", content: "123"},
            ]
        };
    },
    methods: {
        pushMessage(mess) {
            if (this.ws != null) {
                mess = mess.trim();
                if (mess !== '')
                    mess = mess + '\n';
                this.ws.send(mess);
                this.messages.push({from: "me", content: mess.trim()});
                this.waitingOnUs = false;
            }

            // this.messages.push({from: "me", content: mess});
            // setTimeout(() => {
            //     this.messages.push({from: "game", content: "guess 123"});
            //     setTimeout(() => {
            //         this.messages.push({from: "game", content: "guess 456"});
            //     }, 50);
            //     setTimeout(() => {
            //         this.messages.push({from: "game", content: "guess 789"});
            //     }, 50)
            // }, 500);
        },
        connectToGame(matchCode) {
            if (this.ws) {
                console.log("Already plaing game?");
                return;
            }

            const ws = new WebSocket("ws://" + window.location.host + "/ws-api/game-join?match=" + encodeURIComponent(matchCode));
            // const ws = new WebSocket("ws://localhost:18081/ws-api/game-join?match=" + encodeURIComponent(matchCode));
            ws.onclose = (ev) => {
                console.log("Closed ws due to " + ev.reason + " (" + ev.statusCode + ")");
                if (this.ws === ws)
                    this.ws = null;
            };
            ws.onopen = (ev) => {
                console.log("Got connection in WS");
                this.messages = [
                    {from: "system", content: "Connected to " + matchCode},
                ];
                this.ws = ws;
            };
            ws.onerror = (ev) => {
                console.log("Got error on WS:", ev)
            };
            ws.onmessage = (ev) => {
                console.log("Got message: " + ev.data);
                this.messages.push({from: "game", content: ev.data});
                this.waitingOnUs = true;
            };
        },
        startSingleplayer() {
            this.connectToGame(this.game + ";S");
        }
    }
};
</script>