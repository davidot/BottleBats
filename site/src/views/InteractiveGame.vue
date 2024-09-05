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
                        <button @click="startGame">Start game!</button>
                    </td>
                </tr>
            </tbody>
        </table>
        <div>
            <label for="game-join-code">Join game</label><input type="text" id="game-join-code" v-model="gameJoinCode"><button @click="joinGame">Join!</button>
        </div>
    </div>
    <hr>
    <div class="foldable">
        <div v-if="!gameFolded">
            <span v-if="!hasRunningGame || messages.length === 0">
                Waiting for game to start...
            </span>
            <component v-else :is="gameComponent" :messages="messages" @suggestion="setSuggestion" @sendMessage="sendMessage"/>
        </div>
    </div>
    <hr>
    <Console :messages="messages" :waiting-on-us="waitingOnUs" :suggestion="suggestion" @sendMessage="sendMessage"/>
</template>

<script>
import { endpoint } from "@/http";
import Console from "./Console.vue";

// Game imports
import GuessGame from "@/games/GuessGame.vue"
import TickTackToeGame from "@/games/TickTackToeGame.vue"



export default {
    components: {
        Console,
        GuessGame,
        TickTackToeGame,
    },
    props: {
        game: String,
    },
    mounted() {
        endpoint.get("/game-info/" + this.game)
            .then((val) => {
                console.log("Got val: ", val.data);
                this.gameSetupConfig = val.data;
                for (let i = 0; i < val.data.numPlayers; ++i) {
                    this.pickedAlgos.push("");
                }
            });
        setTimeout(() => {
            return;

            this.gameSetupConfig = {
                numPlayers: 5,
                availableAlgos: ['internal'],
                gameBaseName: 'Guess',
            }

            const lines = [
                "result 750 lower",
                "result 335 incorrect",
                "result 334 incorrect",
                "result 335 incorrect",
                "result 346 incorrect",
                "result 250 higher",
                "result 336 incorrect",
                "result 335 incorrect",
                "result 336 incorrect",
                "result 347 incorrect",
                "result 700 lower",
                "result 337 incorrect",
                "result 336 incorrect",
                "result 337 incorrect",
                "result 348 incorrect",
                "result 400 higher",
            ];

            this.addMessage({from: "system", content: "Guess game started!"});

            const doMessage = () => {
                if (lines.length > 0) {
                    const mess = lines.shift();
                    this.addMessage({from: "game", content: mess});
                    setTimeout(doMessage, 1500);
                }
            };

            setTimeout(doMessage, 150);

        }, 500);
    },
    computed: {
        hasRunningGame() {
            return this.ws != null;
        },
        gameComponent() {
            if (!this.gameSetupConfig.gameBaseName)
                return null;

            return this.gameSetupConfig.gameBaseName + 'Game';
        }
    },
    data() {
        return {
            ws: null,
            waitingOnUs: false,
            suggestion: '',
            gameFolded: false,
            gameSetupConfig: {
                numPlayers: -1,
                availableAlgos: [],
                gameBaseName: null,
            },
            pickedAlgos: [],
            gameJoinCode: '',
            messages: []
        };
    },
    methods: {
        addMessage(message) {
            this.messages.push(message);
        },
        sendMessage(mess) {
            if (this.ws != null) {
                mess = mess.trim();
                if (mess !== '')
                    mess = mess + '\n';
                this.ws.send(mess);
                this.messages.push({from: "user", content: mess.trim()});
                this.waitingOnUs = false;
            }
        },
        connectToGame(matchCode) {
            if (this.ws) {
                console.log("Already plaing game?");
                return;
            }

            const ws = new WebSocket("ws://" + window.location.host + "/ws-api/game-join?match=" + encodeURIComponent(matchCode));
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
                console.log("Got message: ", JSON.parse(ev.data));
                const messageList = JSON.parse(ev.data);
                for (const message of messageList) {
                    if (message.type === 'game-message') {
                    this.addMessage({from: "game", content: message.content});
                } else if (message.type === 'you-are-up') {
                    this.waitingOnUs = true;
                } else if (message.type === 'system') {
                    this.addMessage({from: "system", content: message.content});
} else {
                        this.addMessage({from: "system", content: 'Unknown message?: ' + JSON.stringify(message)});
                    }
                }
                
            };
        },
        startSingleplayer() {
            this.connectToGame(this.game + ";S");
        },
        joinGame() {
            this.connectToGame(this.gameJoinCode);
        },
        setSuggestion(hint) {
            this.suggestion = hint;
        },
        startGame() {
            endpoint.post("/setup-game/" + this.game, this.pickedAlgos)
                .then(v => {
                    console.log('success', v);
                }).catch(e => {
                    console.log('fail :(', e);
                });
        }
    }
};
</script>