/**
 * Copyright Soramitsu Co., Ltd. 2017 All Rights Reserved.
 * http://soramitsu.co.jp
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <boost/filesystem.hpp>
#include "ametsuchi/config.hpp"
#include "cli/defaults.hpp"
#include "cli/flags.hpp"
#include "cli/handler/all.hpp"
#include "torii/config.hpp"
#include "util/filesystem.hpp"

using namespace iroha;
using namespace std::literals::string_literals;

// IROHA_VERSION should be defined at compile-time with -DIROHA_VERSION=abcd
#ifdef IROHA_VERSION
// it is a preprocessor trick, which converts X to "X" (const char*)
// refer to https://stackoverflow.com/a/240370/1953079
// example: https://ideone.com/F9g67M
#define STRINGIFY2(X) #X
#define STRINGIFY(X) STRINGIFY2(X)
#define IROHA_VERSION_STR STRINGIFY(IROHA_VERSION)
#else
#define IROHA_VERSION_STR "undefined"
#endif

int main(int argc, char *argv[]) {
  /// DEFAULTS
  iroha::config::Peer peer{};
  iroha::ametsuchi::config::Ametsuchi ametsuchi{};
  iroha::torii::config::Torii torii{};
  iroha::config::Cryptography crypto{};
  iroha::config::OtherOptions other{};
  std::string genesis_path;  // no default value

  /// OPTIONS
  CLI::App main("iroha - simple decentralized ledger"s);
  main.require_subcommand(1);
  main.add_config();  // adds ini config. more:
                      // https://github.com/CLIUtils/CLI11
  main.add_flag("-v,--version"s,
                [&argv](size_t) {
                  std::cout << argv[0] << " version " IROHA_VERSION_STR "\n";
                  std::exit(EXIT_SUCCESS);
                },
                "Current version"s);

  flag::addPeerFlags(&main, &peer, &torii, &crypto);
  flag::addPostgresFlags(&main, &ametsuchi.postgres);
  flag::addRedisFlags(&main, &ametsuchi.redis);
  flag::addBlockStorageFlags(&main, &ametsuchi.blockStorage);
  flag::addOtherOptionsFlags(&main, &other);

  // start
  auto start = main.add_subcommand("start"s, "Start iroha"s);
  start->set_callback(
      [&]() { cli::handler::start(ametsuchi, crypto, other, peer, torii); });

  // ledger
  auto ledger =
      main.add_subcommand("ledger"s, "Manage ledger"s)->require_subcommand(1);

  // ledger create
  auto ledger_create = ledger->add_subcommand(
      "create"s, "Create new network with given genesis block"s);
  flag::addCreateLedgerFlags(ledger_create, &genesis_path);
  ledger_create->set_callback(
      [&]() { cli::handler::ledger::create(&ametsuchi, genesis_path); });

  // ledger clear
  auto ledger_clear =
      ledger->add_subcommand("clear"s,
                             "Clear peer's ledger"s,
                             false /* disable help for this command */);
  ledger_clear->set_callback(
      [&]() { cli::handler::ledger::clear(&ametsuchi); });

  // config
  auto config =
      main.add_subcommand("config"s, "Dump current configuration"s, false);
  config->set_callback([&]() {
    cli::handler::config::config(&ametsuchi, &crypto, &other, &peer, &torii);
  });

  // this macro parses config and executes according callback handler
  CLI11_PARSE(main, argc, argv);

  std::exit(EXIT_SUCCESS);
}
