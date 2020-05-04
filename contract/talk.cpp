#include <eosio/eosio.hpp>

// Message table
struct [[eosio::table("message"), eosio::contract("talk")]] message {
    uint64_t    id       = {}; // Non-0
    uint64_t    reply_to = {}; // Non-0 if this is a reply
    eosio::name user     = {};
    std::string content  = {};
    uint32_t    likes    = {};

    uint64_t primary_key() const { return id; }
    uint64_t get_reply_to() const { return reply_to; }
};

static uint128_t getLikeKey(uint64_t id, const eosio::name& user)
{
    uint128_t temp;
    temp = id;
    temp = temp << 64;
    return temp | user.value;
};

// Like table
struct [[eosio::table("like"), eosio::contract("talk")]] like {
    uint64_t    id  = {}; // Non-0
    eosio::name user     = {};

    uint128_t primary_key() const 
    {
        return getLikeKey(id, user);
    }
};

using message_table = eosio::multi_index<
    "message"_n, message, eosio::indexed_by<"by.reply.to"_n, eosio::const_mem_fun<message, uint64_t, &message::get_reply_to>>>;

using like_table = eosio::multi_index<
    "like"_n, like, eosio::indexed_by<"by.post.id"_n, eosio::const_mem_fun<like, uint128_t, &like::primary_key>>>;

// The contract
class talk : eosio::contract {
  public:
    // Use contract's constructor
    using contract::contract;

    // Post a message
    [[eosio::action]] void post(uint64_t id, uint64_t reply_to, eosio::name user, const std::string& content) {
        message_table table{get_self(), 0};

        // Check user
        require_auth(user);

        // Check reply_to exists
        if (reply_to)
            table.get(reply_to);

        // Create an ID if user didn't specify one
        eosio::check(id < 1'000'000'000ull, "user-specified id is too big");
        if (!id)
            id = std::max(table.available_primary_key(), 1'000'000'000ull);

        // Record the message
        table.emplace(get_self(), [&](auto& message) {
            message.id       = id;
            message.reply_to = reply_to;
            message.user     = user;
            message.content  = content;
            message.likes    = 0;
        });
    }

    // Post a like
    [[eosio::action]] void like(uint64_t id, eosio::name user) {
        like_table likes{get_self(), 0};
        message_table messages{get_self(), 0};

        // Check user
        require_auth(user);
        //check if post exists
        auto it_m = messages.find(id);
        eosio::check(it_m != messages.end(), "Message not found.");

        uint128_t like_key = getLikeKey(id, user);
        auto it = likes.find(like_key);
        //if post was liked than unlike it
        if (it != likes.end())
        {
            likes.erase(it);
            messages.modify(it_m, user, [&]( auto& message ) {
                message.likes = message.likes - 1;
            });
            eosio::print("post ", id, " was unliked by ", user);
        }
        else
        {
            // Record the message
            likes.emplace(get_self(), [&](auto& like) {
                like.id       = id;
                like.user     = user;
            });
            messages.modify(it_m, user, [&]( auto& message ) {
                message.likes = message.likes + 1;
            });
            eosio::print("post ", id, " was liked by ", user);
        }
    }

    [[eosio::action]] void verifylikes(uint64_t id, uint32_t num) {
        message_table table{get_self(), 0};
        eosio::check(table.get(id).likes == num, "Invalid likes number");
    }
};