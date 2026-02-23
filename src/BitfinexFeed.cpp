#include "ob/BitfinexFeed.hpp"
#include "ob/OrderBook.hpp"
#include "ob/Order.hpp"

#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <nlohmann/json.hpp>

#include <atomic>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

namespace ob {
using json = nlohmann::json;

static int price_to_cents(double price) {
    return static_cast<int>(llround(price * 100.0));
}

static int64_t btc_to_sats(double btc) {
    return static_cast<int64_t>(llround(btc * 100000000.0));
}

BitfinexFeed::BitfinexFeed(OrderBook& book) : book_(book) {}

void BitfinexFeed::run(const std::string& pair) {
    ix::initNetSystem();

    ix::WebSocket ws;
    ws.setUrl("wss://api.bitfinex.com/ws/2");

    std::atomic<int> seq{0};
    std::atomic<int> chan_id{-1};

    // Build subscribe payload once
    json sub = {
        {"event","subscribe"},
        {"channel","book"},
        {"pair", pair},
        {"prec","R0"}
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

        if (msg->type != ix::WebSocketMessageType::Message) return;

        json j;
        try { j = json::parse(msg->str); }
        catch (...) { return; }

        // TEMP: uncomment if you want to see raw traffic
        // std::cerr << "[bitfinex] msg: " << msg->str << "\n";

        // subscription / event objects
        if (j.is_object()) {
            if (j.value("event", "") == "subscribed" &&
                j.value("channel", "") == "book" &&
                j.value("prec", "") == "R0") {
                chan_id = j.value("chanId", -1);
                std::cerr << "[bitfinex] subscribed chanId=" << chan_id.load() << "\n";
            }
            return;
        }

        // data arrays
        if (!j.is_array() || j.size() < 2) return;

        const int ch = j[0].get<int>();
        if (chan_id.load() != -1 && ch != chan_id.load()) return;

        // heartbeat: [chanId, "hb"]
        if (j.size() == 2 && j[1].is_string() && j[1].get<std::string>() == "hb") return;

        const int cur_seq = seq.fetch_add(1);

        // Snapshot: [chanId, [ [id, price, amount], ... ] ]
        if (j.size() == 2 && j[1].is_array() && j[1].size() > 0 && j[1][0].is_array()) {
            book_.clear_book();

            for (const auto& row : j[1]) {
                if (!row.is_array() || row.size() < 3) continue;

                const int order_id = row[0].get<int>();
                const double price = row[1].get<double>();
                const double amount = row[2].get<double>();

                if (price == 0.0) continue;

                const OrderSide side = (amount > 0.0) ? OrderSide::BID : OrderSide::ASK;
                const int price_tick = price_to_cents(price);
                const int64_t qty = btc_to_sats(std::fabs(amount));

                Order o{order_id, side, price_tick, qty, cur_seq};
                book_.upsert_by_id(o);
            }

            std::cerr << "[bitfinex] snapshot loaded seq=" << cur_seq << "\n";
            return;
        }

        // Update: [chanId, id, price, amount]
        if (j.size() == 4) {
            const int order_id = j[1].get<int>();
            const double price = j[2].get<double>();
            const double amount = j[3].get<double>();

            // Delete when price==0
            if (price == 0.0) {
                book_.erase_by_id(order_id);
                return;
            }

            const OrderSide side = (amount > 0.0) ? OrderSide::BID : OrderSide::ASK;
            const int price_tick = price_to_cents(price);
            const int64_t qty = btc_to_sats(std::fabs(amount));

            Order o{order_id, side, price_tick, qty, cur_seq};
            book_.upsert_by_id(o);
            return;
        }
    });

    ws.start();
    std::cerr << "[bitfinex] connecting...\n";

    while (true) std::this_thread::sleep_for(std::chrono::seconds(1));
}

} // namespace ob