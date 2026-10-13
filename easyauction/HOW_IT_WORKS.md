# How this auction works

It is a **batch auction** with a single clearing price: everyone who gets filled
pays the same price.

## Terms
- **Auctioning token** – what is for sale.
- **Bidding token** – what bidders pay with.
- **Order** – packed into 32 bytes: `userId | buyAmount | sellAmount`. A bidder's
  order says "I give `sellAmount` bidding tokens and want at least `buyAmount`
  auctioning tokens". Price = `sellAmount / buyAmount`; higher is better.

## Lifecycle
1. **Initiate** (`initiateAuction`, factory only). The factory deposits the
   auctioned amount plus the fee. The auctioneer's order is stored as
   `(minBuyAmount, auctionedSellAmount)` – the lowest acceptable price.
2. **Order placement** until `auctionEndDate` (`placeSellOrders`). Each order
   must beat the minimum price and exceed `minimumBiddingAmountPerOrder`.
   Orders live in a sorted linked list; the caller supplies the previous node
   (`_prevSellOrders`) so insertion costs O(1) instead of a search. Bidding
   tokens are escrowed.
3. **Cancellation** is allowed until `orderCancellationEndDate`
   (`cancelSellOrders`) – a refund of the bidding tokens.
4. **Solution submission** after the end date. Anyone may call
   `precalculateSellAmountSum` to sum orders from the best price downwards in
   chunks (so big order books can be processed over several transactions).
   `settleAuction` (factory only in this version) finishes the sum, finds where
   supply meets demand and stores the **clearing order**.
5. **Claim** (`claimFromParticipantOrder`). Each order is compared with the
   clearing order:
   - better price → fully filled: gets `sellAmount * num / den` auctioning tokens;
   - equal to the clearing order → partially filled (`volumeClearingPriceOrder`),
     rest refunded;
   - worse price → fully refunded in bidding tokens.

## Clearing price cases in `settleAuction`
Walk orders from best to worst price, accumulating `currentBidSum`, until the
bids cover the full auctioned amount at that order's price.
- **[13]** Covered, and the marginal order is only partially needed → clearing
  order is that order, with a reduced volume.
- **[14]** Covered with room to spare → price lies strictly between the last two
  orders; a synthetic clearing order is created.
- **[15]** Never covered but total bids exceed the minimum price implied total →
  price is set so all bids are used.
- **[16]** Total bids too small even at the minimum price → auction only
  partially fills at the auctioneer's minimum price.
- If total bids < `minFundingThreshold`, the auction fails: bidders get refunds
  and the auctioneer gets the auctioning tokens (plus fee) back.

## Fees
`feeNumerator / 1000` of the auctioned amount (max 1.5%), snapshotted at
initiation, paid to `feeReceiverUserId` only for the portion actually sold.

## What this version changes vs. the original Gnosis contract
- `initiateAuction`, `settleAuction`, `settleAuctionAtomically` are restricted to
  `subjectFactory`, which the owner sets.
- Everything else (orders, claims, fees) follows the original design.
