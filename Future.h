#pragma once

#if __has_include(<coroutine>)
#include <coroutine>
namespace n_coro = ::std;
#elif __has_include(<experimental/coroutine>)
#include <experimental/coroutine>
namespace n_coro = ::std::experimental;
#endif

#include <iostream>
#include <memory>
#include <vector>

template <typename T> struct SharedValue {
public:
  void stage_value(T const &value) {
    value_ = value;
    is_set = true;
  }

  void resume_all() {
    for (auto h : handles) {
      h.resume();
    }
  }
  bool await_ready() const noexcept { return is_set; }

  void await_suspend(n_coro::coroutine_handle<> handle) {
    handles.push_back(handle);
  }

  T await_resume() const noexcept { return value_; }

private:
  T value_;
  bool is_set;
  std::vector<n_coro::coroutine_handle<>> handles;
};

template <typename T> struct promise;

template <typename T> struct future {
public:
  future() : state(std::make_shared<SharedValue<T>>()){};
  future(future const &other) : state(other.state) {}

  ~future() = default;

  bool await_ready() noexcept { return state->await_ready(); }

  void await_suspend(n_coro::coroutine_handle<> handle) {
    state->await_suspend(handle);
  }

  T await_resume() noexcept { return state->await_resume(); }

  struct promise_type {
    promise<T> p;

    promise_type() {}

    future<T> get_return_object() noexcept { return p.get_future(); }

    void return_value(T const &v) {
      p.set_value(v);
    }

    void unhandled_exception() const noexcept {
      // holder.set_exception(std::current_exception());
    }

    n_coro::suspend_never initial_suspend() const noexcept { return {}; }

    n_coro::suspend_never final_suspend() noexcept { return {}; }
  };

private:
  friend struct promise<T>;

  std::shared_ptr<SharedValue<T>> state;
  future(std::shared_ptr<SharedValue<T>> const &state) : state(state) {}
  future(std::shared_ptr<SharedValue<T>> &&state) : state(std::move(state)) {}
};

template <typename T> struct promise {
public:
  promise() : state(std::make_shared<SharedValue<T>>()) {}

  promise(promise const &other) : state(other.state){};

  ~promise() = default;

  future<T> get_future() { return {state}; }

  void set_value(T const &value) {
    state->stage_value(value);
    state->resume_all();
  }

  void stage_value(T const &value) { state->stage_value(value); }

  void resume_all() { state->resume_all(); }

private:
  std::shared_ptr<SharedValue<T>> state;
  promise(std::shared_ptr<SharedValue<T>> const &state) : state(state) {}
  promise(std::shared_ptr<SharedValue<T>> &&state) : state(std::move(state)) {}
};