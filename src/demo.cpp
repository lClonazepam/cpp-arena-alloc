#include "arena.hpp"

#include <iostream>
#include <string>

struct Node {
  int v;
  Node* next;
};

int main() {
  mem::Arena a(4096);
  auto cp = a.checkpoint();

  Node* head = nullptr;
  for (int i = 0; i < 1000; ++i) {
    auto* n = a.create<Node>(Node{i, head});
    head = n;
  }
  std::cout << "after build live=" << a.live_objects()
            << " blocks=" << a.block_count() << "\n";

  int sum = 0;
  for (auto* p = head; p; p = p->next) sum += p->v;
  std::cout << "sum=" << sum << "\n";

  a.rewind(cp);
  std::cout << "after rewind live=" << a.live_objects() << "\n";

  // reuse + free-list path
  int* x = a.create<int>(42);
  a.deallocate(x, sizeof(int));
  int* y = static_cast<int*>(a.allocate(sizeof(int), alignof(int)));
  *y = 7;
  std::cout << "recycled value write ok y=" << *y << "\n";
}
