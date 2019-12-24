/*
    Copyright (C) 2017 gxb

    This file is part of gxb-core.

    gxb-core is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    gxb-core is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with gxb-core.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <boost/test/unit_test.hpp>

#include <graphene/chain/database.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/hardfork.hpp>

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/market_object.hpp>
#include <graphene/chain/vesting_balance_object.hpp>
#include <graphene/chain/withdraw_permission_object.hpp>
#include <graphene/chain/witness_object.hpp>

#include <fc/crypto/digest.hpp>

#include "../common/database_fixture.hpp"

using namespace graphene::chain;
using namespace graphene::chain::test;

BOOST_FIXTURE_TEST_SUITE( staking_tests, database_fixture )

BOOST_AUTO_TEST_CASE( staking_create_test )
{
    //TODO
}

BOOST_AUTO_TEST_CASE( staking_update_test )
{
    //TODO
}

BOOST_AUTO_TEST_CASE( staking_claim_test )
{
    //TODO
}

BOOST_AUTO_TEST_CASE( wit_commission_set_test )
{
    //TODO
}

BOOST_AUTO_TEST_CASE( wit_banned_remove_test )
{
    //TODO
}

BOOST_AUTO_TEST_CASE( switch_node_test )
{
    //TODO
}

BOOST_AUTO_TEST_CASE( voted_reward_test )
{
    //TODO
}

BOOST_AUTO_TEST_SUITE_END()