<template>
<input v-model="pieceShowIndex" type="number" step="1">

{{ hoverPos }} {{ selectedRotation }} {{ lockState }}

<TransitionGroup name="board" tag="div" class="board" @mousemove="mouseMoved" @mouseleave="mouseLeft" @wheel="mouseScroll" @click="mouseClicked" @keyup="keyUp">
    <table key="base" class="base">
        <tr v-for="(r, i) in tableField">
            <td v-for="(c, j) in r" 
                :class="[c !== '-' && `${c}-instant`, (i === hoverPos.row && j === hoverPos.col) && 'cursor']" 
                @click="tableField[i][j] = 'red'" 
                :data-row="i" :data-col="j">
            </td>
        </tr>
    </table>

    <BlokusPiece v-for="p in placedPieces.slice(0, Math.max(0, pieceShowIndex))" 
        :key="'piece' + p.player + '-' + p.startCol + '-' + p.startRow" 
        :class="['piece', p.player]" :piece="p" />

    <BlokusPiece v-if="hoverPiece != null" key="cursor-piece"
        :class="['cursor-piece', 'instant-piece', lockState === 1 && 'locked']"
        :piece="hoverPiece" />

</TransitionGroup>

<div class="piece-gallery">
    <div v-for="(p, i) in pieces" :class="['gallery-piece', pieceIndex === i && 'selected']">
        <BlokusPiece :piece="p" />
    </div>
    <div v-for="(p, i) in pieces" :class="['gallery-name', pieceIndex === i && 'selected']">
        {{ p.name }}
    </div>
</div>

<BlokusPieceLink>
    Click here to download json file
</BlokusPieceLink>

<input type="text" value="hoi">

</template>
  
<script setup>
import BlokusPiece from '@/components/blokus/BlokusPiece.vue';
import BlokusPieceLink from '@/components/blokus/BlokusPieceLink.vue';
import rawPieces from '@/components/blokus/pieces.json';
import { computed, onMounted, onUnmounted, ref, watch } from 'vue'

const emit = defineEmits(['suggestion', 'sendMessage']);

const fieldSize = 20;

const tableField = ref([]);
for (let i = 0; i < fieldSize; ++i) {
    const row = [];
    tableField.value.push(row);
    for (let j = 0; j < fieldSize; ++j) {
        row.push('-');
    }
}

const pieceShowIndex = ref(0);

const placedPieces = computed(() => {
    const connectedBoard = new Array(fieldSize);

    let nextComponent = 0;
    const remap = new Map();
    const firstPart = new Map();

    for (let i = 0; i < fieldSize; ++i) {
        connectedBoard[i] = new Array(fieldSize);
        connectedBoard[i].fill(-1);
        for (let j = 0; j < fieldSize; ++j) {
            const cellColor = tableField.value[i][j];
            if (cellColor === '-') {
                continue;
            }

            const topSame = i > 0 && tableField.value[i - 1][j] === cellColor;
            const leftSame = j > 0 && tableField.value[i][j - 1] === cellColor;

            if (topSame && leftSame) {
                const topGroup = remap.get(connectedBoard[i - 1][j]) ?? connectedBoard[i - 1][j];
                const leftGroup = remap.get(connectedBoard[i][j - 1]) ?? connectedBoard[i][j - 1];
                const minNum = Math.min(topGroup, leftGroup);
                const maxNum = Math.max(topGroup, leftGroup);
                connectedBoard[i][j] = minNum;
                if (minNum !== maxNum) {
                    let mapTo = remap.get(minNum);
                    if (mapTo == null)
                        mapTo = minNum;

                    remap.set(maxNum, mapTo);
                    // FIXME: Potentially have to remap stuff mapping to us...
                }
            } else if (topSame) {
                connectedBoard[i][j] = connectedBoard[i - 1][j];
                if (connectedBoard[i][j] === -1)
                    throw new RangeError("If not empty should have positve component number (top)");
            } else if (leftSame) {
                connectedBoard[i][j] = connectedBoard[i][j - 1];
                if (connectedBoard[i][j] === -1)
                    throw new RangeError("If not empty should have positve component number (left)");
            } else {
                connectedBoard[i][j] = nextComponent++;
                firstPart.set(connectedBoard[i][j], [i, j]);
            }
        }
    }

    const pieceMapping = new Map();

    for (let i = 0; i < fieldSize; ++i) {
        for (let j = 0; j < fieldSize; ++j) {
            let value = connectedBoard[i][j];
            if (value === -1)
                continue;

            // Remap (bounded incase of cycles although those should never happen)
            for (let k = 0; k < 3; ++k) {
                let mappedTo = remap.get(value);
                if (mappedTo == null)
                    break;
                value = mappedTo;
            }

            const pieceBounds = pieceMapping.get(value) ?? { minI: i, minJ: j, maxI: i, maxJ: j, pieces: [], player: tableField.value[i][j] };
            // minI cannot be improved as we go through i first
            if (i > pieceBounds.maxI) {
                pieceBounds.maxI = i;
            }
            if (j < pieceBounds.minJ) {
                pieceBounds.minJ = j;
            }
            if (j > pieceBounds.maxJ) {
                pieceBounds.maxJ = j;
            }
            pieceBounds.pieces.push([i, j]);
            pieceMapping.set(value, pieceBounds);
        }
    }

    return [...pieceMapping.values()].map(p => {
        return {
            top: p.minI,
            height: p.maxI - p.minI + 1,
            left: p.minJ,
            width: p.maxJ - p.minJ + 1,
            player: p.player,
            active: new Set(p.pieces.map(coord => `${coord[0] - p.minI},${coord[1] - p.minJ}`)),
        }
    });
});

const pieces = Object.entries(rawPieces).map(([key, value]) => {
    return Object.assign({name: key}, value);
});

const pieceIndex = ref(0);
const selectedRotation = ref(0);
const lockState = ref(0);

function rotatePiece(basePiece, rotation) {
    const flippedWidthHeight = rotation > 3;

    const piece = {
        width: flippedWidthHeight ? basePiece.height : basePiece.width,
        height: flippedWidthHeight ? basePiece.width : basePiece.height,
        active: new Set(),
        name: basePiece.name,
        rotation,
    };

    for (let y = 0; y < basePiece.height; ++y) {
        for (let x = 0; x < basePiece.width; ++x){
            const on = basePiece.onOff[y * basePiece.width + x] === 1;
            if (!on)
                continue;

            const minusX = basePiece.width - x - 1;
            const minusY = basePiece.height - y - 1;

            switch (rotation) {
            case 0:
                piece.active.add(`${y},${x}`);
                break;
            case 1:
                piece.active.add(`${y},${minusX}`);
                break;
            case 2:
                piece.active.add(`${minusY},${x}`);
                break;
            case 3:
                piece.active.add(`${minusY},${minusX}`);
                break;
            case 4:
                piece.active.add(`${x},${y}`);
                break;
            case 5:
                piece.active.add(`${x},${minusY}`);
                break;
            case 6:
                piece.active.add(`${minusX},${y}`);
                break;
            case 7:
                piece.active.add(`${minusX},${minusY}`);
                break;
            }
        }
    }

    return piece;
}

const selectedPiece = computed(() => {
    if (pieceIndex.value < 0 || pieceIndex.value >= pieces.length)
        return null;

    const basePiece = pieces[pieceIndex.value];
    const rotation = basePiece.uniqueRotations[selectedRotation.value % basePiece.uniqueRotations.length];

    return rotatePiece(basePiece, rotation);
});

watch(pieceIndex, () => {
    selectedRotation.value = 0;
    lockState.value = 0
});

let hoverPos = ref({row: -1, col: -1});


const hoverPiece = computed(() => {
    const hoverCol = hoverPos.value.col;
    const hoverRow = hoverPos.value.row;
    if (hoverCol < 0 || hoverRow < 0 || selectedPiece.value == null)
        return null;

    if ((hoverCol + selectedPiece.value.width > fieldSize) || (hoverRow + selectedPiece.value.height > fieldSize))
        return null;

    return Object.assign({
        top: hoverRow,
        left: hoverCol,
    }, selectedPiece.value);
});

function resetHover() {
    lockState.value = 0;
    hoverPos.value = {
        row: -1, 
        col: -1,
    };
}

function mouseMoved(e) {
    if (e.target == null || !e.target.hasAttribute('data-row')) {
        return;
    }
    if (lockState.value === 1)
        return;

    hoverPos.value = {
        row: parseInt(e.target.getAttribute('data-row')), 
        col: parseInt(e.target.getAttribute('data-col')) 
    };
}

function mouseLeft() {
    if (lockState.value === 1)
        return;

    hoverPos.value = {
        row: -1, 
        col: -1,
    };
}

function mouseScroll(e) {
    if (e.ctrlKey || e.deltaY == 0) {
        return;
    }
    e.preventDefault();

    let steps = e.deltaY / 120;
    if (Math.abs(steps) < 1) {
        if (steps < 0)
            steps = -1;
        else
            steps = 1;
    }

    steps = Math.trunc(steps);

    if (e.shiftKey) {
        let newIndex = pieceIndex.value - steps;
        if (newIndex < 0)
            newIndex = 0;
        else if (newIndex >= pieces.length)
            newIndex = pieces.length - 1;
        
        pieceIndex.value = newIndex;
    } else {
        let newRotation = (selectedRotation.value -steps) % 8;
        if (newRotation < 0)
            newRotation += 8;
        
        selectedRotation.value = newRotation;
    }
}

function createMessage() {
    if (hoverPiece.value == null)
        return 'invalid';

    const piece = hoverPiece.value;

    const basePiece = pieces[pieceIndex.value];
    const top = hoverPiece.value.top;
    const left = hoverPiece.value.left;
    
    return `place ${piece.name}-${piece.rotation} @ ${top},${left}`;
}

function mouseClicked(e) {
    if (hoverPiece.value == null || e.button != 0) {
        console.log('No hoverpice!', e.button);
        return;
    }
    if (lockState.value === 1) {
        // Send message
        console.log('send', createMessage());
        emit('sendMessage', createMessage());
        resetHover();

    } else if (lockState.value === 0) {
        // Suggestion
        console.log('sugg', createMessage());
        emit('suggestion', createMessage());
        lockState.value = 1;
    }
}

function keyUp(e) {
    if (e.target != null && e.target != document.body)
        return;

    if (e.key === "Escape") {
        resetHover();
    } else if (e.key === "Enter" && lockState.value === 1) {
        console.log('send', createMessage());
        emit('sendMessage', createMessage());
        resetHover();
    }
}


onMounted(() => {
    document.addEventListener('keyup', keyUp);
});
onUnmounted(() => {
    document.removeEventListener('keyup', keyUp);
});


</script>
  
<style>
.red-instant {
    background-color: rgba(255, 0, 0, 0.2);
}

.blue-instant {
    background-color: rgba(0, 0, 255, 0.2);
}

.green-instant {
    background-color: rgba(0, 255, 0, 0.2);
}

.yellow-instant {
    background-color: rgba(255, 255, 0, 0.2);
}

.board {
    position: relative;
    width: fit-content;
}

.board > table {
    border-collapse: collapse;
}


.board > table:not(.base) {
    position: absolute;
}

.board > table td {
    width: 32px;
    height: 32px;
    min-width: 32px;
    min-height: 32px;
    padding: 0;
    box-sizing: border-box;
    border: 1px solid black;
}


.piece-gallery > .gallery-piece {
    width: var(--piece-size);
    height: var(--piece-size);
    display: flex;
    align-items: center;
    justify-content: center;
    border: 3px solid transparent;
    border-bottom: 0;
}

.piece-gallery > .gallery-piece.selected {
    border-color: purple;
}

.piece-gallery > .gallery-name.selected {
    border-color: purple;
}

.gallery-name {
    width: var(--piece-size);
    display: flex;
    justify-content: center;
    border: 3px solid transparent;
    border-top: 0;
}

.piece-gallery table {
    border-collapse: collapse;
    position: relative;
}

.piece-gallery table td {
    width: var(--grid-size);
    height: var(--grid-size);
    padding: 0;
    box-sizing: border-box;
    border: 1px solid black;
}

.piece-gallery {
    display: grid;
    /* MAKE VARIABLE!*/
    grid-template-columns: repeat(21, 1fr);
    grid-template-rows: var(--piece-size) 1em;
    column-gap: 1px;
    align-items: center;
    justify-items: center;

    --piece-size: 64px;
    --grid-size: calc((var(--piece-size) - 10px) / 5);
}

.cursor-piece td {
    background-color: turquoise;
}

.cursor-piece.locked td {
    background-color: purple;
}

table.piece {
    position: absolute;
}

td.empty {
    background-color: transparent !important;
    width: 0 !important;
    height: 0 !important;
    border: 0 !important;
}

.red td {
    background-color: red;
}

.blue td {
    background-color: blue;
}

.green td {
    background-color: green;
}

.yellow td {
    background-color: yellow;
}

.board-enter-from:not(.instant-piece),
.board-leave-to:not(.instant-piece) {
    opacity: 0.5;
}

.board-enter-from.red {
    top: 100% !important;
    left: 100% !important;
}

.board-enter-from.blue {
    top: 100% !important;
    left: 0 !important;
}

.board-enter-from.green {
    top: 0% !important;
    left: 100% !important;
}

.board-enter-from.yellow {
    top: 0% !important;
    left: 0% !important;
}

.board-enter-active:not(.instant-piece),
.board-leave-active:not(.instant-piece) {
transition: all 2s ease;
}

</style>