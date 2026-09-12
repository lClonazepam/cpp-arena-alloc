# cpp-arena-alloc

A scoped bump-pointer arena with:

- Power-of-two growing blocks via `::operator new`
- Alignment-aware bump
- Checkpoints + `rewind()` for frame/request scoped allocation
- A simple free-list so individual objects can be recycled without resetting the whole arena

This is the allocator pattern used in compilers, game frame heaps, and RPC request arenas. The interesting parts are alignment math, lifetime (placement new vs destructor policy), and not paying a lock per allocation.

## Build

```bash
cmake -S . -B build && cmake --build build -j
./build/arena_demo
```
