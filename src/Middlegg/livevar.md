### Live Variable Analysis

#### Computing GEN for each basic block

1. initialize the set `GEN[B]` to empty set
2. initialize a empty set `S` that holds visited stores
3. iterate over all instruction in basic block B, and for each instruction `i` do the following:
    1. If instruction `i` is store instruction add it to `S`
    2. If instruction `i` is a load instruction that reads from a ptr `%n`
        1. if there are no store instruction in `S` also references the same ptr `%n` that the current instruction reads from
            1. Add the load instruction `i` to `GEN[B]`

#### Computing KILL for each basic block

1. Compute the set `G` o all load instructions in the given funtino before computing the KILL for all basic blocks.
2. initialize `KILL[B]` to empty set
3. for each instruction `i` in basic block `B`:
    1. If `i` is a store instruction to any ptr `%n`, add all the load instruction in `G` that reads from the same ptr `%n` and gets killed by `i` to `KILL[B]`


#### Computing IN and OUT sets
1. Initialize the `OUT[B]` of each basic block `B` to the empty set.
2. For each basic block B set `IN[B] = GEN[B]`
3. change = True
4. While (change) do the following:
    1. change = False
    2. for each basic block
        1. `OUT[B]` = $\bigcup_{S \in \text{successor}(B)}$ `IN[S]`
        2. `oldin` = `IN[B]`
        3. `IN[B]` = `GEN[B]` $\cup$(`IN[B] - KILL[B]`)
        4. if `IN[B] != oldin`: change = True
        

#### what to do after the above computations
Once the IN and OUT are computed for each basic block we need to check for store instructions that can be deleted from the program. To do so we walk through each basic block `B` and do the following:

1. `R = OUT[B]`
2. for every instruction `i` in `B` do the following:
    1. If `i` is a store instruction that references the ptr `%n`
        1. for every load instruction in `R`, if no load instruction not read from ptr `%n`
            1. mark the store instruction `i` to be deleted
3. Delete all marked store instruction
