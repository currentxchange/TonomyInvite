# TonomyInvite
🤝 Join Tonomy with cXc and get rewarded BLUX you can use to upvote. 

## Overview
TonomyInvite is a gamified multi-level referral system built on the Tonomy blockchain, designed to incentivize community growth and engagement. The system rewards users for inviting others to join the Tonomy ecosystem, with a unique scoring mechanism that encourages sustainable growth.


## Features
- 🔗 Multi-level referral tracking
- 🎵 Tetrahedral scoring system with position-based bonuses
- 💎 BLUX token rewards for successful referrals
- ⏱️ Rate-limited invitations to prevent spam
- 🔒 Age verification for new accounts
- 📊 Comprehensive statistics tracking

## For Users

### How to Join
1. Ensure your Tonomy account is at least 30 days old
2. Get an invite code from an existing member in [cXc's Telegram]
3. Use the invite code to register through the Tonomy platform

### Earning Rewards
- Each successful referral earns you points
- Points accumulate based on your referral network
- Higher positions in the tetrahedral scoring system earn bonus rewards
- Rewards can be claimed at any time

### Reward Calculation
- Base reward: BLUX tokens based on your points
- Bonus: Additional percentage based on your tetrahedral position
- Example: If you're in position 3, you get a 3% bonus on your base reward

## For Project Owners

### Technical Architecture
The system is built as a smart contract on the Tonomy blockchain with the following components:

#### Core Tables
- `adopters`: Tracks registered users and their referral statistics
- `config`: Stores contract-wide configuration parameters
- `stats`: Maintains global referral and user statistics

#### Key Functions
- `redeeminvite`: Registers new users with referral tracking
- `claimreward`: Processes reward claims with bonus calculations
- `update_scores`: Manages the multi-level scoring system
- `setconfig`: Administrative configuration management

### Configuration Parameters
- `min_account_age_days`: 30 days minimum account age default
- `invite_rate_seconds`: 3600 seconds (1 hour) defaul tcooldown between invites
- `max_referral_depth`: defauly 5 levels deep referral chain
- `multiplier`: 100 (1.0x base score)
- `reward_rate`: 100 (1.00 YOUR per point)
- `reward_symbol`: YOUR token symbol
- `token_contract`: YOUR token contract address

### Security Features
- Account age verification
- Rate limiting for invitations
- Admin-controlled configuration
- Secure token distribution

## Development

### Prerequisites
- Tonomy blockchain development environment
- EOSIO.CDT (Contract Development Toolkit)
- Tonomy account for deployment

### Building
```bash
cd contract
eosio-cpp -o invitono.wasm invitono.cpp
```

## Disclaimer
This software is provided "as is", without warranty of any kind, express or implied, including but not limited to the warranties of merchantability, fitness for a particular purpose and noninfringement. In no event shall the authors or copyright holders be liable for any claim, damages or other liability, whether in an action of contract, tort or otherwise, arising from, out of or in connection with the software or the use or other dealings in the software.
