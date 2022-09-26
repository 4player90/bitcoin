// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BTCSIGNALS_H
#define BITCOIN_BTCSIGNALS_H


#if 1

#include <boost/signals2/connection.hpp>
#include <boost/signals2/optional_last_value.hpp>
#include <boost/signals2/signal.hpp>

namespace btcsignals = boost::signals2;

#else

template <typename T, typename U>
struct signal
{
};

struct connection
{
};

#endif

#endif // BITCOIN_BTCSIGNALS_H
