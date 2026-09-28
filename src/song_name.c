#define SONG_FIRST_INDEX 1
#define SONG_LAST_INDEX UINT64_MAX

static void SongNameAppendChar(SongName *songName, char c) {
    ASSERT(songName->length < SONG_NAME_CAPACITY);

    songName->chars[songName->length++] = c;
}

static uint8_t Consume4Bits(uint64_t *state) {
    uint8_t bits = (*state) & 0x0F;
    (*state) >>= 4;
    return bits;
}

#define SONG_NAME_CONSONANT_OPTIONS 16
#define SONG_NAME_VOWEL_OPTIONS 4

static void GetSongName(uint64_t songIndex, SongName *songName) {
    songName->length = 0;

    uint64_t state = DeterministicRandom(songIndex);

    const static char consonants[SONG_NAME_CONSONANT_OPTIONS] = {
        'r', 't', 'p', 's',
        'd', 'f', 'g', 'h',
        'j', 'k', 'l', 'z',
        'v', 'b', 'n', 'm',
    };

    const static char vowels[SONG_NAME_VOWEL_OPTIONS] = {
        'a', 'o', 'u', 'i',
    };

    const int chunkSize = 4;
    const int chunkAmount = 4;

    for (int chunkIndex = 0; chunkIndex < chunkAmount; chunkIndex++) {
        for (int i = 0; i < chunkSize; i++) {
            int consonantIndex = Consume4Bits(&state);
            ASSERT(consonantIndex < SONG_NAME_CONSONANT_OPTIONS);

            int vowelChunkOffset = -chunkIndex;
            if (vowelChunkOffset < 0) {
                vowelChunkOffset += chunkAmount;
            }
            int vowelIndex = (vowelChunkOffset + i) % SONG_NAME_VOWEL_OPTIONS;
            ASSERT(vowelIndex < SONG_NAME_VOWEL_OPTIONS);

            char consonant = consonants[consonantIndex];
            char vowel = vowels[vowelIndex];

            SongNameAppendChar(songName, consonant);
            SongNameAppendChar(songName, vowel);
        }
        bool isLastIndex = chunkIndex == (chunkAmount - 1);
        if (isLastIndex) {
            SongNameAppendChar(songName, '\0');
        } else {
            // spaces between chunks
            SongNameAppendChar(songName, '-');
        }
    }

    ASSERT(songName->length == SONG_NAME_CAPACITY);

    // capitalize first char
    songName->chars[0] -= 32;
}

