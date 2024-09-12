export const withCasesData = {
    type: 'cases',
    cases: [],
    allStats: {},
    bots: [],
};

export const noCasesData = {
    type: 'no-case',
    allStats: {
        numGamesPlayed: {
            higherIntent: 'neutral',
            name: 'Games played',
        },
        gamesWon: {
            higherIntent: 'positive',
            name: 'Games won',
        },
    },
    bots: [
        {
            id: 0,
            name: 'Bot 123 long name hahahahhahahahahahahahahahahahahah',
            image: '123.png',
            stats: {
                numGamesPlayed: 123,
                gamesWon: 12,
            },
        },
        {
            id: 2,
            name: 'Bot bad :(',
            image: '789.png',
            stats: {
                numGamesPlayed: 12,
                gamesWon: 1,
            },
        },
    ]
};


export const noCasesData2 = {
    type: 'no-case',
    allStats: {
        numGamesPlayed: {
            higherIntent: 'neutral',
            name: 'Games played',
        },
        gamesWon: {
            higherIntent: 'positive',
            name: 'Games won',
        },
    },
    bots: [
        {
            id: 0,
            name: 'Bot 123 long name hahahahhahahahahahahahahahahahahah',
            image: '123.png',
            stats: {
                numGamesPlayed: 123,
                gamesWon: 12,
            },
        },
        {
            id: 1,
            name: 'Bot good',
            image: '456.png',
            stats: {
                numGamesPlayed: 34,
                gamesWon: 30,
            },
        },
        {
            id: 2,
            name: 'Bot bad :(',
            image: '789.png',
            stats: {
                numGamesPlayed: 12,
                gamesWon: 1,
            },
        },
    ]
};