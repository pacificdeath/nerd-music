typedef struct WeightedId {
    int id;
    int weight;
} WeightedId;

#define RANDOM_WEIGHTS_CAPACITY 8
typedef struct WeightedIdList {
    WeightedId items[RANDOM_WEIGHTS_CAPACITY];
    int count;
} WeightedIdList;

static uint64_t NextRandom() {
    uint64_t x = sharedState->randomState;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    sharedState->randomState = x;
    return x;
}

static void AppendWeightedId(WeightedIdList *list, WeightedId item) {
    ASSERT(list->count >= 0);
    ASSERT(list->count < RANDOM_WEIGHTS_CAPACITY);

    list->items[list->count++] = item;
}

static WeightedId GetRandomIdByWeight(const WeightedIdList *list) {
    ASSERT(list->count > 0);
    ASSERT(list->count <= RANDOM_WEIGHTS_CAPACITY);

    int totalWeight = 0;
    for (int i = 0; i < list->count; i++) {
        int weight = list->items[i].weight;
        ASSERT(weight > 0);
        totalWeight += weight;
    }

    ASSERT(totalWeight > 0);

    int random = NextRandom() % totalWeight;

    for (int i = 0; i < list->count; i++) {
        int weight = list->items[i].weight;

        if (random < weight) {
            return list->items[i];
        }

        random -= weight;
    }

    ASSERT(false);
    return (WeightedId){0};
}

