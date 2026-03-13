#include "ob/BitfinexFeed.hpp"
#include "ob/OrderBook.hpp"
#include "ob/Order.hpp"

#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <nlohmann/json.hpp>

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>

namespace ob {

using json = nlohmann::json;

static std::uint32_t price_to_tick(double price) {
    return static_cast<std::uint32_t>(llround(price * 100.0));
}

static std::uint64_t btc_to_sats(double btc) {
    return static_cast<std::uint64_t>(llround(btc * 100000000.0));
}

BitfinexFeed::BitfinexFeed(OrderBook& book) : book_(book) {}

void BitfinexFeed::run(const std::string& pair) {
    ix::initNetSystem();

    ix::WebSocket ws;
    ws.setUrl("wss://api.bitfinex.com/ws/2");

    std::atomic<std::uint64_t> seq{0};
    std::atomic<int> chan_id{-1};

    json sub = {
        {"event", "subscribe"},
        {"channel", "book"},
        {"pair", pair},
        {"prec", "R0"}
    };
    const std::string sub_msg = sub.dump();

    ws.setOnMessageCallback([&](const ix::WebSocketMessagePtr& msg) {
        if (msg->type == ix::WebSocketMessageType::Open) {
            std::cerr << "[bitfinex] open\n";
            ws.send(sub_msg);
            std::cerr << "[bitfinex] sent subscribe\n";
            return;
        }

        if (msg->type == ix::WebSocketMessageType::Error) {
            std::cerr << "[bitfinex] error: " << msg->errorInfo.reason << "\n";
            return;
        }

        if (msg->type != ix::WebSocketMessageType::Message) {
            return;
        }

        json j;
        try {
            j = json::parse(msg->str);
        } catch (...) {
            return;
        }

        if (j.is_object()) {
            if (j.value("event", "") == "subscribed" &&
                j.value("channel", "") == "book" &&
                j.value("prec", "") == "R0") {
                chan_id = j.value("chanId", -1);
                std::cerr << "[bitfinex] subscribed chanId=" << chan_id.load() << "\n";
            }
            return;
        }

        if (!j.is_array() || j.size() < 2) {
            return;
        }

        const int ch = j[0].get<int>();
        if (chan_id.load() != -1 && ch != chan_id.load()) {
            return;
        }

        if (j.size() == 2 && j[1].is_string() && j[1].get<std::string>() == "hb") {
            return;
        }

        // Snapshot:
        // [chanId, [ [id, price, amount], ... ] ]
        if (j.size() == 2 && j[1].is_array() && !j[1].empty() && j[1][0].is_array()) {
            book_.clear_book();

            for (const auto& row : j[1]) {
                if (!row.is_array() || row.size() < 3) {
                    continue;
                }

                const std::int64_t raw_order_id = row[0].get<std::int64_t>();
                const double price = row[1].get<double>();
                const double amount = row[2].get<double>();

                if (price == 0.0) {
                    continue;
                }

                const OrderSide side = (amount > 0.0) ? OrderSide::BID : OrderSide::ASK;

                Order o{
                    .order_id = static_cast<std::uint64_t>(raw_order_id),
                    .seq = ++seq,
                    .quantity = btc_to_sats(std::fabs(amount)),
                    .price_tick = price_to_tick(price),
                    .side = side
                };

                book_.upsert_by_id(o);
            }

            std::cerr << "[bitfinex] snapshot loaded\n";
            return;
        }

        // Update:
        // [chanId, id, price, amount]
        if (j.size() == 4) {
            const std::int64_t raw_order_id = j[1].get<std::int64_t>();
            const double price = j[2].get<double>();
            const double amount = j[3].get<double>();

            const std::uint64_t order_id = static_cast<std::uint64_t>(raw_order_id);

            // delete
            if (price == 0.0) {
                book_.cancel_order(order_id);
                return;
            }

            const OrderSide side = (amount > 0.0) ? OrderSide::BID : OrderSide::ASK;

            Order o{
                .order_id = order_id,
                .seq = ++seq,
                .quantity = btc_to_sats(std::fabs(amount)),
                .price_tick = price_to_tick(price),
                .side = side
            };

            book_.upsert_by_id(o);
            return;
        }
    });

    ws.start();
    std::cerr << "[bitfinex] connecting...\n";

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

} // namespace ob