#pragma once

#include <iostream>
#include <vector>

#if __has_include(<coroutine>)
#include <coroutine>
namespace n_coro = ::std;
#elif __has_include(<experimental/coroutine>)
#include <experimental/coroutine>
namespace n_coro = ::std::experimental;
#endif

#include <assert.h>
#define ASSERT assert

template <class T> struct SAV {
  int promises;
  int futures;

  std::vector<n_coro::coroutine_handle<>> handles;

private:
  typename std::aligned_storage<sizeof(T), __alignof(T)>::type value_storage;
  T &value() { return *(T *)&value_storage; }

  // value > 0 = error
  enum State { UNSET = -3, NEVER, SET };

  // TODO: Add Error state

public:
  SAV(int futures, int promises)
      : promises(promises), futures(futures), state(UNSET) {}
  ~SAV() {
    if (state == SET)
      value().~T();
  }

  bool isSet() const { return state > NEVER; }
  bool canBeSet() const { return state == UNSET; }
  bool isError() const { return state > SET; }

  T const &get() const {
    ASSERT(isSet());
    if (isError())
      throw state;
    return *(T const *)&value_storage;
  }

  template <class U> void send(U &&value) {
    ASSERT(canBeSet());
    new (&value_storage) T(std::forward<U>(value));
    this->state = SET;
    // while (Callback<T>::next != this) {
    //   Callback<T>::next->fire(this->value());
    // }

    fireCallbacks();

    // Fire callbacks
  }

  template <class U> void set(U &&value) {
    ASSERT(canBeSet());
    new (&value_storage) T(std::forward<U>(value));
    this->state = SET;

    // std::cout << "value set" << std::endl;
    // while (Callback<T>::next != this) {
    //   Callback<T>::next->fire(this->value());
    // }

    // fireCallbacks();

    // Fire callbacks
  }

  // void sendError(Error err) {
  // 	ASSERT(canBeSet() && int16_t(err.code()) > 0);
  // 	this->error_state = err;
  // 	while (Callback<T>::next != this) {
  // 		Callback<T>::next->error(err);
  // 	}
  // }

  void add_callback(n_coro::coroutine_handle<> handle) {
    handles.push_back(handle);
  }

  void fireCallbacks() {
    // std::cerr << "callbacks fire" << std::endl;

    for (auto h : handles) {
      h.resume();
    }
  }

  void addPromiseRef() { promises++; }
  void addFutureRef() { futures++; }

  void delPromiseRef() {
    if (promises == 1) {
      if (futures && canBeSet()) {
        // TODO:
        // sendError(broken_promise());

        std::cerr << "broken promise" << std::endl;

        ASSERT(promises ==
               1); // Once there is only one promise, there is no one else with
                   // the right to change the promise reference count
      }
      promises = 0;
      if (!futures)
        destroy();
    } else
      --promises;
  }
  void delFutureRef() {
    if (!--futures) {
      if (promises)
        cancel();
      else
        destroy();
    }
  }

  int getFutureReferenceCount() const { return futures; }
  int getPromiseReferenceCount() const { return promises; }

  virtual void destroy() {
    // std::cerr << "delete()" << std::endl;
    delete this;
  }

  virtual void cancel() {
    // std::cerr << "cancel()" << std::endl;
  }

private:
  State state;
};

template <class T> class Promise;

template <class T> class Future {
public:
  T const &get() const { return sav->get(); }
  T getValue() const { return get(); }

  bool isValid() const { return sav != 0; }
  bool isReady() const { return sav->isSet(); }
  bool isError() const { return sav->isError(); }
  // returns true if get can be called on this future (counterpart of canBeSet
  // on Promises)
  bool canGet() const { return isValid() && isReady() && !isError(); }
  // Error &getError() const {
  //   ASSERT(isError());
  //   return sav->error_state;
  // }

  Future() : sav(0) {}
  Future(const Future<T> &rhs) : sav(rhs.sav) {
    if (sav)
      sav->addFutureRef();
    // if (sav->endpoint.isValid()) std::cerr << "Future copied for " <<
    // sav->endpoint.key << std::endl;
  }
  Future(Future<T> &&rhs) noexcept : sav(rhs.sav) {
    rhs.sav = 0;
    // if (sav->endpoint.isValid()) std::cerr << "Future moved for " <<
    // sav->endpoint.key << std::endl;
  }
  Future(const T &presentValue) : sav(new SAV<T>(1, 0)) {
    sav->send(presentValue);
  }
  Future(T &&presentValue) : sav(new SAV<T>(1, 0)) {
    sav->send(std::move(presentValue));
  }
  // Future(Never) : sav(new SAV<T>(1, 0)) { sav->send(Never()); }
  // Future(const Error &error) : sav(new SAV<T>(1, 0)) { sav->sendError(error);
  // }

  ~Future() {
    // if (sav && sav->endpoint.isValid()) std::cerr << "Future destroyed for "
    // << sav->endpoint.key << std::endl;
    if (sav)
      sav->delFutureRef();
  }
  void operator=(const Future<T> &rhs) {
    if (rhs.sav)
      rhs.sav->addFutureRef();
    if (sav)
      sav->delFutureRef();
    sav = rhs.sav;
  }
  void operator=(Future<T> &&rhs) noexcept {
    if (sav != rhs.sav) {
      if (sav)
        sav->delFutureRef();
      sav = rhs.sav;
      rhs.sav = 0;
    }
  }
  bool operator==(const Future &rhs) { return rhs.sav == sav; }
  bool operator!=(const Future &rhs) { return rhs.sav != sav; }

  void cancel() {
    if (sav)
      sav->cancel();
  }

  // void addCallbackAndClear(Callback<T> *cb) {
  //   sav->addCallbackAndDelFutureRef(cb);
  //   sav = 0;
  // }

  // void addYieldedCallbackAndClear(Callback<T> *cb) {
  //   sav->addYieldedCallbackAndDelFutureRef(cb);
  //   sav = 0;
  // }

  // void addCallbackChainAndClear(Callback<T> *cb) {
  //   sav->addCallbackChainAndDelFutureRef(cb);
  //   sav = 0;
  // }

  int getFutureReferenceCount() const { return sav->getFutureReferenceCount(); }
  int getPromiseReferenceCount() const {
    return sav->getPromiseReferenceCount();
  }

  explicit Future(SAV<T> *sav) : sav(sav) {
    // if (sav->endpoint.isValid()) std::cerr << "Future created for " <<
    // sav->endpoint.key << std::endl;
  }

private:
  SAV<T> *sav;
  friend class Promise<T>;

  // C++ 20 Coroutine

public:
  struct promise_type {

    Promise<T> p;

    Future get_return_object() { return p.getFuture(); }

    std::experimental::suspend_never initial_suspend() { return {}; }

    std::experimental::suspend_never final_suspend() noexcept {
      // std::cout << "Final suspend" << std::endl;
      p.fireCallbacks(); 
      return {};
    }

    void unhandled_exception() {
      // TODO:
      // p.sendError
    }

    // void return_void() {}
    // ---- OR ----
    // void return_value(T value) { p.send(value); }

    template <class U> void return_value(U &&value) const {
      // std::cout << "value set" << std::endl;
      p.set(std::forward<U>(value));
      // p.send(std::forward<U>(value));
    }
  };

  bool await_ready() noexcept { return canGet(); }

  void await_suspend(n_coro::coroutine_handle<> handle) {
    // state->await_suspend(handle);

    sav->add_callback(handle);
  }

  // T await_resume() noexcept { return  }
  T const &await_resume() const { return sav->get(); }

private:
  std::experimental::coroutine_handle<promise_type> h_;
};

template <class T> class Promise final {
public:
  template <class U> void send(U &&value) const {
    sav->send(std::forward<U>(value));
  }

  template <class U> void set(U &&value) const {
    sav->set(std::forward<U>(value));
  }

  void fireCallbacks() { sav->fireCallbacks(); }
  template <class E> void sendError(const E &exc) const { sav->sendError(exc); }

  Future<T> getFuture() const {
    sav->addFutureRef();
    return Future<T>(sav);
  }
  bool isSet() const { return sav->isSet(); }
  bool canBeSet() const { return sav->canBeSet(); }
  bool isError() const { return sav->isError(); }

  bool isValid() const { return sav != nullptr; }
  Promise() : sav(new SAV<T>(0, 1)) {}
  Promise(const Promise &rhs) : sav(rhs.sav) { sav->addPromiseRef(); }
  Promise(Promise &&rhs) noexcept : sav(rhs.sav) { rhs.sav = 0; }

  ~Promise() {
    if (sav)
      sav->delPromiseRef();
  }

  void operator=(const Promise &rhs) {
    if (rhs.sav)
      rhs.sav->addPromiseRef();
    if (sav)
      sav->delPromiseRef();
    sav = rhs.sav;
  }
  void operator=(Promise &&rhs) noexcept {
    if (sav != rhs.sav) {
      if (sav)
        sav->delPromiseRef();
      sav = rhs.sav;
      rhs.sav = 0;
    }
  }
  void reset() { *this = Promise<T>(); }
  void swap(Promise &other) { std::swap(sav, other.sav); }

  // Beware, these operations are very unsafe
  SAV<T> *extractRawPointer() {
    auto ptr = sav;
    sav = nullptr;
    return ptr;
  }
  explicit Promise<T>(SAV<T> *ptr) : sav(ptr) {}

  int getFutureReferenceCount() const { return sav->getFutureReferenceCount(); }
  int getPromiseReferenceCount() const {
    return sav->getPromiseReferenceCount();
  }

private:
  SAV<T> *sav;
};
