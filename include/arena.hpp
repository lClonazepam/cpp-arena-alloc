#pragma once

#include <cstddef>
#include <cstdint>
#include <new>
#include <utility>
#include <vector>

namespace mem {

// Bump-pointer arena with rewind checkpoints and a size-class free list
// for objects that outlive a checkpoint. Blocks are power-of-two aligned.
class Arena {
 public:
  explicit Arena(std::size_t block_size = 1 << 20) : block_size_(block_size) {
    grow();
  }

  Arena(const Arena&) = delete;
  Arena& operator=(const Arena&) = delete;

  ~Arena() {
    for (auto* b : blocks_) ::operator delete(b);
  }

  void* allocate(std::size_t bytes, std::size_t align = alignof(std::max_align_t)) {
    if (bytes == 0) bytes = 1;
    if (void* recycled = try_recycle(bytes, align)) return recycled;

    std::uintptr_t cur = reinterpret_cast<std::uintptr_t>(ptr_);
    std::uintptr_t aligned = (cur + align - 1) & ~(align - 1);
    std::uintptr_t end = reinterpret_cast<std::uintptr_t>(end_);
    if (aligned + bytes > end) {
      grow(bytes + align);
      cur = reinterpret_cast<std::uintptr_t>(ptr_);
      aligned = (cur + align - 1) & ~(align - 1);
    }
    ptr_ = reinterpret_cast<char*>(aligned + bytes);
    ++live_;
    return reinterpret_cast<void*>(aligned);
  }

  template <typename T, typename... Args>
  T* create(Args&&... args) {
    void* p = allocate(sizeof(T), alignof(T));
    return new (p) T(std::forward<Args>(args)...);
  }

  struct Checkpoint {
    std::size_t block_index;
    char* ptr;
    std::size_t live;
  };

  Checkpoint checkpoint() const {
    return {blocks_.size() - 1, ptr_, live_};
  }

  // Rewind bump pointer. Does not run destructors — caller must.
  void rewind(Checkpoint cp) {
    ptr_ = cp.ptr;
    live_ = cp.live;
    // Keep extra blocks allocated; just stop using them until next grow.
    while (blocks_.size() > cp.block_index + 1) {
      // leave capacity; cheap rewind is the point
      break;
    }
  }

  void deallocate(void* p, std::size_t bytes) {
    if (!p) return;
    free_list_.push_back({p, bytes});
    if (live_ > 0) --live_;
  }

  std::size_t live_objects() const { return live_; }
  std::size_t block_count() const { return blocks_.size(); }

 private:
  struct FreeNode {
    void* ptr;
    std::size_t size;
  };

  void* try_recycle(std::size_t bytes, std::size_t align) {
    for (std::size_t i = 0; i < free_list_.size(); ++i) {
      auto node = free_list_[i];
      auto addr = reinterpret_cast<std::uintptr_t>(node.ptr);
      if (node.size >= bytes && (addr % align) == 0) {
        free_list_[i] = free_list_.back();
        free_list_.pop_back();
        ++live_;
        return node.ptr;
      }
    }
    return nullptr;
  }

  void grow(std::size_t at_least = 0) {
    std::size_t sz = block_size_;
    while (sz < at_least) sz *= 2;
    void* b = ::operator new(sz);
    blocks_.push_back(static_cast<char*>(b));
    ptr_ = static_cast<char*>(b);
    end_ = ptr_ + sz;
  }

  std::size_t block_size_;
  std::vector<char*> blocks_;
  char* ptr_ = nullptr;
  char* end_ = nullptr;
  std::size_t live_ = 0;
  std::vector<FreeNode> free_list_;
};

}  // namespace mem
