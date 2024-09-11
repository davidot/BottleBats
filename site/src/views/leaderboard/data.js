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
        },
        gamesWon: {
            reaction: 'positive',
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
        }
    ]
};