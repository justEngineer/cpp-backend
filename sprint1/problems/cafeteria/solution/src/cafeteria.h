#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <iostream>
#include <memory>
#include <mutex>
#include <syncstream>

#include "hotdog.h"
#include "ingredients.h"
#include "result.h"

namespace net = boost::asio;
namespace sys = boost::system;
namespace ph = std::placeholders;
using namespace std::chrono;
using namespace std::literals;
using Timer = net::steady_timer;

class ThreadChecker {
   public:
    explicit ThreadChecker(std::atomic_int& counter) : counter_{counter} {}

    ThreadChecker(const ThreadChecker&) = delete;
    ThreadChecker& operator=(const ThreadChecker&) = delete;

    ~ThreadChecker() {
        // assert выстрелит, если между вызовом конструктора и деструктора
        // значение expected_counter_ изменится
        assert(expected_counter_ == counter_);
    }

   private:
    std::atomic_int& counter_;
    int expected_counter_ = ++counter_;
};

// Функция-обработчик операции приготовления хот-дога
using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;

class Order : public std::enable_shared_from_this<Order> {
   public:
    Order(net::io_context& io, int id, std::shared_ptr<GasCooker> gas_cooker, HotDogHandler handler,
          std::shared_ptr<Sausage> sausage, std::shared_ptr<Bread> bread)
        : io_{io},
          id_{id},
          handler_{std::move(handler)},
          gas_cooker_(gas_cooker),
          sausage_(std::move(sausage)),
          bread_(std::move(bread)) {}

    // Запускает асинхронное выполнение заказа
    void Execute() {
        sausage_->StartFry(*gas_cooker_, [self = shared_from_this()] {
            self->sausage_timer_.expires_after(1500ms);
            self->sausage_timer_.async_wait([self = self->shared_from_this()](sys::error_code ec) {
                if (ec)
                    std::cout << ec.message() << "\n";
                std::lock_guard lock(self->mutex_);
                self->sausage_->StopFry();
                if (self->bread_->is_cooked) {
                    std::osyncstream os{std::cout};
                    os << "HotDog is ready! id: " << self->id_ << std::endl;
                    self->handler_(HotDog(self->id_, self->sausage_, self->bread_));
                }
            });
        });
        bread_->StartBake(*gas_cooker_, [self = shared_from_this()] {
            self->bread_timer_.expires_after(1000ms);
            self->bread_timer_.async_wait([self = self->shared_from_this()](sys::error_code ec) {
                if (ec)
                    std::cout << ec.message() << "\n";
                std::lock_guard lock(self->mutex_);
                self->bread_->StopBaking();
                if (self->sausage_->is_cooked) {
                    std::osyncstream os{std::cout};
                    os << "HotDog is ready! id: " << self->id_ << std::endl;
                    self->handler_(HotDog(self->id_, self->sausage_, self->bread_));
                }
            });
        });
    }
    std::atomic_int counter_{0};
    net::io_context& io_;
    int id_;
    HotDogHandler handler_;
    std::mutex mutex_;
    Timer bread_timer_{io_};
    Timer sausage_timer_{io_};
    std::shared_ptr<GasCooker> gas_cooker_;
    std::shared_ptr<Sausage> sausage_;
    std::shared_ptr<Bread> bread_;
};

// Класс "Кафетерий". Готовит хот-доги
class Cafeteria {
   public:
    explicit Cafeteria(net::io_context& io) : io_{io} {}

    // Асинхронно готовит хот-дог и вызывает handler, как только хот-дог будет готов.
    // Этот метод может быть вызван из произвольного потока
    void OrderHotDog(HotDogHandler handler) {
        // TODO: Реализуйте метод самостоятельно
        // При необходимости реализуйте дополнительные классы
        std::unique_lock storeLock_(storeMtx_);
        auto new_order = std::make_shared<Order>(io_, next_order_id_++, gas_cooker_, std::move(handler),
                                                 store_.GetSausage(), store_.GetBread());
        storeLock_.unlock();
        new_order->Execute();
    }

   private:
    net::io_context& io_;
    // Используется для создания ингредиентов хот-дога
    Store store_;
    std::mutex storeMtx_;
    // Газовая плита. По условию задачи в кафетерии есть только одна газовая плита на 8 горелок
    // Используйте её для приготовления ингредиентов хот-дога.
    // Плита создаётся с помощью make_shared, так как GasCooker унаследован от
    // enable_shared_from_this.
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);
    int next_order_id_ = 0;
};
