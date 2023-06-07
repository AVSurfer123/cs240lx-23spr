#include <stdio.h>
#include <stdlib.h>
#include <search.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

enum opcode {
    OPCODE_BRANCH = 'B',
    OPCODE_KILL = 'K',
    OPCODE_DEREF = 'D',
};

struct exprmap {
    uint64_t *exprs, *deref_labels;
    size_t n_exprs;
};

// each of these should be ~5-10loc for a basic implementation. if you want to
// go crazy, try keeping it sorted & using binary search (only once you have
// everything working initially!).
uint64_t lookup_exprmap(const struct exprmap set, uint64_t expr) {
    // you can return 0 if you don't find anything
    for (int i = 0; i < set.n_exprs; i++) {
        if (set.exprs[i] == expr) {
            return set.deref_labels[i];
        }
    }
    return 0;
}

struct exprmap copy_exprmap(struct exprmap old, int old_size, int new_size) {
    struct exprmap new;
    new.exprs = malloc(new_size * sizeof(uint64_t));
    new.deref_labels = malloc(new_size * sizeof(uint64_t));
    new.n_exprs = old_size;
    memcpy(new.exprs, old.exprs, old_size * sizeof(uint64_t));
    memcpy(new.deref_labels, old.deref_labels, old_size * sizeof(uint64_t));
    return new;
}

struct exprmap insert_exprmap(const struct exprmap old, uint64_t expr, uint64_t deref_label) {
    if (lookup_exprmap(old, expr)) {
        return old;
    }
    struct exprmap new = copy_exprmap(old, old.n_exprs, old.n_exprs + 1);
    new.exprs[new.n_exprs] = expr;
    new.deref_labels[new.n_exprs] = deref_label;
    new.n_exprs++;
    return new;
}

struct exprmap remove_exprmap(const struct exprmap old, uint64_t expr) {
    struct exprmap new = copy_exprmap(old, old.n_exprs, old.n_exprs);
    for (int i = 0; i < new.n_exprs; i++) {
        if (new.exprs[i] == expr) {
            new.n_exprs--;
            new.exprs[i] = new.exprs[new.n_exprs];
            new.deref_labels[i] = new.deref_labels[new.n_exprs];
            break;
        }
    }
    return new;
}

int subset_exprmap(const struct exprmap small, const struct exprmap big) {
    for (int i = 0; i < small.n_exprs; i++) {
        int found = 0;
        for (int j = 0; j < big.n_exprs; j++) {
            if (small.exprs[i] == big.exprs[j]) {
                found = 1;
                break;
            }
        }
        if (!found) {
            return 0;
        }
    }
    return 1;
}

struct exprmap intersect_exprmaps(const struct exprmap small, const struct exprmap big) {
    struct exprmap new = copy_exprmap(small, 0, small.n_exprs);
    for (int i = 0; i < small.n_exprs; i++) {
        for (int j = 0; j < big.n_exprs; j++) {
            if (small.exprs[i] == big.exprs[j]) {
                new.exprs[new.n_exprs] = small.exprs[i];
                new.deref_labels[new.n_exprs] = small.deref_labels[i];
                new.n_exprs++;
                break;
            }
        }
    }
    return new;
}

struct instr {
    uint64_t label;
    enum opcode opcode;
    uint64_t args[3];
    struct instr *nexts[2];

    int visited;
    struct exprmap always_derefed;
};

void visit(struct instr *instr, struct exprmap derefed) {
    // Update instr->visited, instr->always_derefed, derefed, or return as
    // needed for each of the following cases:
    // (1) instr is NULL
    // (2) this is the first path to reach instr
    // (3) along every prior path to @instr every expression in @derefed has
    //     been derefed
    // (4) there are some expressions in instr->always_derefed that are not in
    //     @derefed along this path
    // should be about 10loc total; use the data structure operations you
    // implemented above!
    if (instr == NULL) {
        return;
    }
    if (!instr->visited) {
        instr->visited = 1;
        instr->always_derefed = derefed;
    }
    else if (subset_exprmap(instr->always_derefed, derefed)) {
        return;
    }
    else {
        instr->always_derefed = intersect_exprmaps(instr->always_derefed, derefed);
    }

    // now actually process the instruction:
    // (1) if it's a kill, then we no longer know anything about instr->args[0]
    // (2) if it's a deref, then we should remember that instr->args[0] has
    //     been derefed for the remainder of this path (at least, until it's
    //     killed later on)
    if (instr->opcode == OPCODE_KILL) {
        instr->always_derefed = remove_exprmap(instr->always_derefed, instr->args[0]);
    }
    else if (instr->opcode == OPCODE_DEREF) {
        instr->always_derefed = insert_exprmap(instr->always_derefed, instr->args[0], instr->label);
    }

    // now recurse on the possible next-instructions. we visit nexts[1] first
    // out of superstition (it's more likely to be NULL and we want to do the
    // most work in the tail recursive call)
    derefed = instr->always_derefed; // feel free to remove if your sln doesn't need this
    visit(instr->nexts[1], derefed);
    visit(instr->nexts[0], derefed);
}

void check(struct instr *instr) {
    if (!instr || instr->opcode != OPCODE_BRANCH) return;
    if (!lookup_exprmap(instr->always_derefed, instr->args[0])) return;
    printf("%lu:%lu\n", lookup_exprmap(instr->always_derefed, instr->args[0]),
           instr->label);
}

int main() {
    // you don't need to modify any of this; it just parses the input IR, then
    // calls visit on the first instruction, then calls check on every
    // instruction.
    struct instr *head = NULL;
    size_t n_instructions = 0, max_label = 0;
    while (!feof(stdin)) {
        struct instr *new = calloc(1, sizeof(*new));
        char opcode;
        if (scanf(" %lu %c", &(new->label), &opcode) != 2) {
            // if this is failing unexpectedly, and you're on a Mac, maybe just
            // remove it? still seemed to work for Manya
            assert(feof(stdin));
            break;
        }
        new->opcode = opcode;
        int n_args = (new->opcode == OPCODE_BRANCH) ? 3 : 1;
        for (size_t i = 0; i < n_args; i++)
            assert(scanf(" %lu", &(new->args[i])) == 1);
        new->nexts[0] = head;
        head = new;
        n_instructions++;
        if (new->label > max_label) max_label = new->label;
        while (!feof(stdin) && fgetc(stdin) != '\n') continue;
    }

    size_t n_labels = max_label + 1;
    struct instr **label2instr = calloc(n_labels, sizeof(*label2instr));

    for (struct instr *instr = head; instr; instr = instr->nexts[0]) {
        assert(!label2instr[instr->label]);
        label2instr[instr->label] = instr;
    }

    struct instr *new_head = NULL;
    for (struct instr *instr = head; instr; ) {
        struct instr *real_next = instr->nexts[0];
        instr->nexts[0] = new_head;
        new_head = instr;
        instr = real_next;
    }
    head = new_head;

    for (size_t i = 0; i < n_labels; i++) {
        struct instr *instr = label2instr[i];
        if (instr && instr->opcode == OPCODE_BRANCH) {
            if (!(label2instr[instr->args[1]]) || !(label2instr[instr->args[2]])) {
                exit(1);
            }
            instr->nexts[0] = label2instr[instr->args[1]];
            instr->nexts[1] = label2instr[instr->args[2]];
        }
    }

    visit(head, (struct exprmap){NULL, NULL, 0});
    for (size_t i = 0; i < n_labels; i++)
        check(label2instr[i]);
}
