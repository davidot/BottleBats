<template>
    <div id="console" @click="maybeFocus">
        <div v-for="message in messages" :class="[message.from, 'message']">
            {{ message.content }}
        </div>
        <span :class="{'input-indicator': true, 'waiting': waitingOnUs}">$> </span>
        <span @select="print">{{ inputText }}</span>
        <span v-if="suggestion != null && suggestion.length > 0" style="color: gray;">{{ suggestion }}</span>
        <span id="text-input">
            <input type="text" v-model="inputText" ref="input" v-on:keyup.enter="onEnter">
        </span>
    </div>
</template>

<script>

export default {
    props: {
        messages: {
            type: Array,
            required: true,
        },
        waitingOnUs: {
            type: Boolean,
            required: true,
        },
        suggestion: {
            type: String,
            required: true,
        }
    },
    mounted() {
        // document.addEventListener("selectionchange", (e) => {
        //     console.log('body selectionchange', e);
        //     console.log(document.getSelection())
        //     console.log(this.$refs.input.selectionStart);
        //     console.log(this.$refs.input.selectionEnd);

        //     if (this.$refs.input.selectionEnd == 6) {
        //         this.$refs.input.selectionEnd = 3;
        //     }
        //     // console.log(document.getSelection().anchorNode)
        //     // console.log(document.getSelection().anchorOffset)
        // })
    },
    data() {
        return {
            inputText: '',
        };
    },
    methods: {
        print(e) {
            console.log('span select')
            console.log(e);
        },
        onEnter() {
            this.$emit('send-message', this.inputText);
            this.inputText = '';
        },
        maybeFocus() {
            // if (this.$refs.input === document.activeElement)
            //     return;
            // const selection = document.getSelection();
            // if (selection.type === "Range")
            //     return;

            // this.$refs.input.focus();
            // selection.setBaseAndExtent(this.$refs.input, 0, this.$refs.input, 1);
            // console.log(document.getSelection());
            // console.log(document.activeElement, this.$refs.input);
            // if (this.$refs.input.)
        }
    }

};
</script>

<style scoped>
#console {
    background-color: black;
    color: white;
    font-family: monospace;
    font-size: larger;
    padding-bottom: 4px;
}
/* :focus-within */
#text-input::before {
    content: '\00a0';
    width: 0.5em;
    height: 1em;
    color: orange;
    border-bottom: 2px solid white;
    animation: blink 1s step-end infinite;
}

#text-input > input {
    /* width: 0px; */
    margin-left: 2em;
    overflow: hidden;
    border: 0;
    outline: 0;
    /* height: 0px; */
    /* opacity: 0; */
}

@keyframes blink {
  from,
  to {
    border-color: transparent;
  }
  50% {
    border-color: #fff;
  }
}

.message {
    padding-left: 1em;
}

.message.me {
    color: green;
}

.message.game {
    color: red;
}

.message.system {
    color: gray;
    text-decoration: underline;
}

.input-indicator.waiting {
    color: yellow;
}
</style>