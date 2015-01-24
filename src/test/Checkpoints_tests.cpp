// Copyright (c) 2011-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//
// Unit tests for block-chain checkpoints
//

#include "checkpoints.h"

#include "chainparams.h"
#include "uint256.h"
#include "test/test_bitcoin.h"

#include <boost/test/unit_test.hpp>

using namespace std;

BOOST_FIXTURE_TEST_SUITE(Checkpoints_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(sanity)
{
    uint256 p11111 = uint256S("0x0000000069e244f73d78e8fd29ba2fd2ed618bd6fa2ee92559f542fdb26e7c1d");
    uint256 p134444 = uint256S("0x00000000000005b12ffd4cd315cd34ffd4a594f430ac814c91184a0d42d2b0fe");
    BOOST_CHECK(Params().Checkpoints().CheckBlock(11111, p11111));
    BOOST_CHECK(Params().Checkpoints().CheckBlock(134444, p134444));

    
    // Wrong hashes at checkpoints should fail:
    BOOST_CHECK(!Params().Checkpoints().CheckBlock(11111, p134444));
    BOOST_CHECK(!Params().Checkpoints().CheckBlock(134444, p11111));

    // ... but any hash not at a checkpoint should succeed:
    BOOST_CHECK(Params().Checkpoints().CheckBlock(11111+1, p134444));
    BOOST_CHECK(Params().Checkpoints().CheckBlock(134444+1, p11111));

    BOOST_CHECK(Params().Checkpoints().GetTotalBlocksEstimate() >= 134444);
}    

BOOST_AUTO_TEST_SUITE_END()
