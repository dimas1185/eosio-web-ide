#include <boost/test/unit_test.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/permission_object.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/testing/tester.hpp>

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;

BOOST_AUTO_TEST_SUITE(talk_tests)

BOOST_AUTO_TEST_CASE(post) try {
    tester t{setup_policy::none};

    // Load contract
    t.create_account(N(talk));
    t.set_code(N(talk), read_wasm("talk.wasm"));
    t.set_abi(N(talk), read_abi("talk.abi").data());
    t.produce_block();

    // Create users
    t.create_account(N(john));
    t.create_account(N(jane));

    // Test "post" action
    t.push_action(
        N(talk), N(post), N(john),
        mutable_variant_object //
        ("id", 1)              //
        ("reply_to", 0)        //
        ("user", "john")       //
        ("content", "post 1")  //
    );
    t.push_action(
        N(talk), N(post), N(jane),
        mutable_variant_object //
        ("id", 2)              //
        ("reply_to", 0)        //
        ("user", "jane")       //
        ("content", "post 2")  //
    );
    t.push_action(
        N(talk), N(post), N(john),
        mutable_variant_object       //
        ("id", 3)                    //
        ("reply_to", 2)              //
        ("user", "john")             //
        ("content", "post 3: reply") //
    );

    // Can't reply to non-existing message
    BOOST_CHECK_THROW(
        [&] {
            t.push_action(
                N(talk), N(post), N(john),
                mutable_variant_object       //
                ("id", 4)                    //
                ("reply_to", 99)             //
                ("user", "john")             //
                ("content", "post 3: reply") //
            );
        }(),
        fc::exception);

    //1-st like
    t.push_action(
        N(talk), N(like), N(john),
        mutable_variant_object       //
        ("id", 2)               //
        ("user", "john")             //
    );

    //2-nd like
    t.push_action(
        N(talk), N(like), N(jane),
        mutable_variant_object       //
        ("id", 2)               //
        ("user", "jane")             //
    );

    //verify we have 2 likes for post 2
    t.push_action(
        N(talk), N(verifylikes), N(jane),
        mutable_variant_object       //
        ("id", 2)               //
        ("num", 2)             //
    );

    //unlike as it was liked before
    //t.push_action(
    //    N(talk), N(like), N(jane),
    //    mutable_variant_object       //
    //    ("id", 2)               //
    //    ("user", "jane")             //
    //);

    // Can't like as other user
    BOOST_CHECK_THROW(
        [&] {
            t.push_action(
                N(talk), N(like), N(john),
                mutable_variant_object       //
                ("id", 2)                    //
                ("user", "jane")             //
            );
        }(),
        fc::exception);

    // Can't like non existing post
    BOOST_CHECK_THROW(
        [&] {
            t.push_action(
                N(talk), N(like), N(john),
                mutable_variant_object       //
                ("id", 4)                    //
                ("user", "john")             //
            );
        }(),
        fc::exception);
    
    //invalid likes number
    BOOST_CHECK_THROW(
        [&] {
            t.push_action(
                N(talk), N(verifylikes), N(john),
                mutable_variant_object       //
                ("id", 2)                    //
                ("num", 3)             //
            );
        }(),
        fc::exception);
}
FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
