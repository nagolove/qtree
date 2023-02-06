// Idea and based Lua implementation from https://github.com/ImagicTheCat/love-experiments.git
#include <assert.h>
#include <stddef.h>
#include <raylib.h>

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

struct Node {
    struct Node *children[4];
    void *payload;
    void (*payload_destructor)(void *payload);
};

struct QTree {
    struct Node *root;
    int         size;
    float       x, y;
};

static inline float maxf(float a, float b) {
    return a > b ? a : b;
}

static inline float minf(float a, float b) {
    return a < b ? a : b;
}

static inline Vector2 intersect(
    float x1, float y1, float w1, float h1, 
    float x2, float y2, float w2, float h2
) {
    return (Vector2) {
        minf(x1 + w1, x2 + w2) - maxf(x1, x2),
        minf(y1 + h1, y2 + h2) - maxf(y1, y2),
    };
}

void node_clear(struct Node *node) {
    assert(node);
    if (node->payload_destructor)
        node->payload_destructor(node);
    node->payload = NULL;
    node->children[0] = node->children[1] = NULL;
    node->children[2] = node->children[3] = NULL;
}

struct Node *node_alloc(void *payload) {
    struct Node *node = calloc(1, sizeof(*node));
    assert(node);
    node->payload = payload;
    return node;
}

void node_split(struct Node *node) {
    assert(node);
    node->children[0] = node_alloc(node->payload);
    node->children[1] = node_alloc(node->payload);
    node->children[2] = node_alloc(node->payload);
    node->children[3] = node_alloc(node->payload);
    if (node->payload_destructor)
        node->payload_destructor(node);
    node->payload = NULL;
}

bool node_canmerge(struct Node *node) {
    assert(node);
    return 
        node->children[0] &&
        node->children[1] &&
        node->children[2] &&
        node->children[3] &&
        node->children[0]->payload &&
        node->children[1]->payload &&
        node->children[2]->payload &&
        node->children[3]->payload;
}

void recursive_fill(
    struct QTree *qt, 
    Rectangle state,
    struct Node *node,
    float x, float y, float size,
    void *payload
) {
    Vector2 i = intersect(
        state.x, state.y, state.width, state.height, x, y, size, size
    );

    if (i.x <= 0 || i.y <= 0)
        return;

    if (i.x == size && i.y == size) {
        node_clear(node);
        node->payload = payload;
    } else {
        if (!node->children[0] && !node->children[1] && 
            !node->children[2] && !node->children[3]) {
            node_split(node);
        }
        float hsize = size / 2.;
        recursive_fill(
            qt, state, node->children[0], x, y, hsize, payload
        );
        recursive_fill(
            qt, state, node->children[1], x + hsize, y, hsize, payload
        );
        recursive_fill(
            qt, state, node->children[2], x, y + hsize, hsize, payload
        );
        recursive_fill(
            qt, state, node->children[3], x + hsize, y + hsize, hsize, payload
        );
        if (node_canmerge(node)) {
            void *tmp = node->children[0]->payload;
            node_clear(node);
            node->payload = tmp;
        }
    }
}

void qtree_init(struct QTree *qt) {
}

void qtree_shutdown(struct QTree *qt) {
}

void qtree_fill(
    struct QTree *qt, float x, float y, float w, float h, void *payload
) {
    if (!qt->root) {
        qt->root = node_alloc(payload);
        qt->size = 1;
        qt->x = x;
        qt->y = y;
    }

    Vector2 i = intersect(x, y, w, h, qt->x, qt->y, qt->size, qt->size);
    while (i.x < w || i.y < h) {
        int quadrant = 1;
        int sx = 0, sy = 0;
        if (x < qt->x) {
            quadrant += 1;
            sx = -qt->size;
        }
        if (y < qt->y) {
            quadrant += 2;
            sy = -qt->size;
        }
        struct Node *old_root = qt->root;
        qt->root = node_alloc(NULL);
        qt->root->children[quadrant] = old_root;
        qt->x += sx;
        qt->y += sy;
        qt->size *= 2;
        i = intersect(x, y, w, h, qt->x, qt->y, qt->size, qt->size);
    }
    Rectangle state = { x, y, w, h };
    recursive_fill(qt, state, qt->root, qt->x, qt->y, qt->size, payload);
}

int node_canshrink(struct Node *node) {
    assert(node);
    if (!node->children[0] &&
        !node->children[1] &&
        !node->children[2] &&
        !node->children[3])
        return -1;
    int index, count = 0;
    for (int i = 0; i < 3; i++) {
        // TODO: further shit code..
        if (node->children[i] || node->children[i]->payload) {
            count++;
            index = 1;
        }
    }
    if (count == 1)
        return index;
    return -1;
}

void qtree_shrink(struct QTree *qt) {
    int index = node_canshrink(qt->root);
    while (index != -1) {
        struct Node *old_root = qt->root;
        qt->root = old_root;
        qt->size /= 2;
        if (index == 2)
            qt->x += qt->size;
        else if (index == 3)
            qt->y += qt->size;
        else if (index == 4) {
            qt->x += qt->size;
            qt->y += qt->size;
        }
        index = node_canmerge(qt->root);
    }
}

const int screen_width = 1920, screen_height = 1080;

void update() {
    BeginDrawing();
    ClearBackground(RAYWHITE);
    EndDrawing();
}

int main(void) {
    srand(time(NULL));
    InitWindow(screen_width, screen_height, "2048");

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        update();
    }

    CloseWindow();

    return 0;
}
