#!/bin/bash

git clone git@github.com:EOSIO/eosio.contracts.git
cd eosio.contracts
git checkout develop
cp ./contracts/eosio.boot/ ../ -ur
git checkout v1.9.2
cp ./contracts/eosio.bios/ ../ -ur
cp ./contracts/demo.tmy/ ../ -ur
cp ./contracts/eosio.token/ ../ -ur
cp ./contracts/vesting.tmy/ ../ -ur
rm eosio.contracts -rf