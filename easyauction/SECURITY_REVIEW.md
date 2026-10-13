# EasyAuction (modified) – security review

Scope: the single contract pasted by the user. It is a modified copy of Gnosis
EasyAuction. The libraries (`IterableOrderedOrderSet`, `IdToAddressBiMap`,
`SafeCast`, `AllowListVerifier`) were **not** provided and were **not** reviewed.
This is a manual read-through; nothing was compiled or run (see README).

## Ranked findings

### 1. Settlement is gated to one address (High – liveness / fund lock)
`settleAuction` and `settleAuctionAtomically` require `msg.sender == subjectFactory`.
Bidders' funds, and the auctioneer's tokens, only move after settlement
(`claimFromParticipantOrder` needs `clearingPriceOrder != 0`). If the factory
is buggy, lacks a call path to `settleAuction`, is paused, or the owner points
`subjectFactory` at a bad address, every running auction is stuck with funds
locked. The original contract lets anyone settle, which is safe because the
result is deterministic.
**Fix:** let anyone call `settleAuction` after the end date; keep factory-only
only for `settleAuctionAtomically` if that is truly needed.

### 2. Owner can swap `subjectFactory` at any time (Medium – centralization)
`setSubjectFactory` has no timelock, no zero-address check and no event. Changing
it mid-auction changes who can settle running auctions. **Fix:** timelock or
multisig owner, emit an event, reject `address(0)`.

### 3. Fee receiver and `feeNumerator` changes affect live auctions (Medium)
`feeNumerator` is snapshotted per auction, but `feeReceiverUserId` is global
(the code comment says so). The owner can redirect fees of in-flight auctions.
Also `setFeeParameters` registers the receiver via `getUserId`, and the default
`feeReceiverUserId = 1` is simply whoever registered first. **Fix:** snapshot
the receiver per auction, or document it clearly; emit an event.

### 4. Unchecked truncating cast in `AuctionCleared` (Low)
`uint96(currentBidSum)` silently truncates. Other paths use `toUint96()` (which
reverts), so truncation could only affect the event, but it makes off-chain
data wrong. **Fix:** `currentBidSum.toUint96()`.

### 5. Array length mismatch is not validated (Low)
`_placeSellOrders` iterates over `_minBuyAmounts.length` and indexes
`_sellAmounts[i]` and `_prevSellOrders[i]`. Shorter arrays revert (OK); longer
`_sellAmounts` / `_prevSellOrders` are silently ignored. **Fix:** require equal
lengths.

### 6. `placeSellOrdersOnBehalf` – payer vs. owner of the order (Low / confirm intent)
Tokens are pulled from `msg.sender`, while the order is credited to
`orderSubmitter`. An attacker cannot steal funds this way (they pay), but they
can create orders in someone else's name, and the allow-list check is applied to
`orderSubmitter`, not the payer. Confirm this is intended.

### 7. Dead code in `settleAuction` auth (Informational)
`|| msg.sender == address(this)` is never true: `settleAuctionAtomically` calls
`settleAuction` as an internal call, so `msg.sender` is still the factory.
Harmless, but misleading.

### 8. `registerUser` is public (Informational)
Anyone can register any address and consume user ids. `numUsers` is `uint64`, so
exhaustion is not realistic; matches the original.

### 9. Token assumptions (Informational)
- Fee-on-transfer / rebasing tokens break accounting (escrow < recorded amount).
- ERC777-style hooks: state is updated before external transfers in claim,
  cancel and settle (checks-effects-interactions) and `settleAuction` sets
  `clearingPriceOrder` before paying out, which blocks re-entering settle.
  No `ReentrancyGuard`; acceptable, but worth a test with a hook token.
- Auctions that raise more than 2^96 units of the bidding token cannot be
  settled (documented in the contract).

### 10. Misc / hygiene
- `initiateAuction` performs the token transfer before the parameter checks
  (harmless since a revert undoes it, but checks-first is cleaner).
- `pragma solidity >=0.6.8` is floating; pin it (e.g. `0.6.12`) for deployment.
- No events for `setSubjectFactory` / `setFeeParameters`.
- The cleared-state "gas refunds" zero `initialAuctionOrder`; `claimFromParticipantOrder`
  does not need it, but any future code reading it after settlement would see 0.

## Things that looked fine
- SafeMath on all arithmetic; `SafeERC20` on all token moves.
- Order removal happens before payouts in `claimFromParticipantOrder`, so an
  order cannot be claimed twice.
- Cancellation requires the caller's `userId` to match the order's.
- Min order size and better-than-initial-price checks bound gas for settlement.

## Suggested next steps
1. Fix #1 (permissionless settle) and #4, #5.
2. Add timelock/event for #2 and #3.
3. Review the unprovided libraries and run the included tests.
