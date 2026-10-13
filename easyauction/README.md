# EasyAuction – fixed copy, review, explainer and tests

- `contracts/EasyAuction.sol` – the pasted contract with the two compile issues
  fixed (`pragma` typo; truncated tail completed with `containsOrder`'s body and
  the closing brace), plus one deliberate change: `settleAuction` is now
  permissionless after the auction ends (review finding #1). The `containsOrder` body
  (`return sellOrders[auctionId].contains(order);`) is reconstructed from the
  original Gnosis contract – please compare with your source.
- `SECURITY_REVIEW.md` – ranked findings.
- `HOW_IT_WORKS.md` – step-by-step explanation.
- `test/EasyAuction.test.js` – Hardhat tests; `contracts/test/MockERC20.sol` – mock token.

## Not verified
Nothing here has been compiled or run. The contract imports libraries that were not
part of the paste (`libraries/IterableOrderedOrderSet.sol`, `IdToAddressBiMap.sol`,
`SafeCast.sol`, `interfaces/AllowListVerifier.sol`). Copy them from the project they
came from (Gnosis `ido-contracts`), then:

    npm install
    npx hardhat test

The tests were written against the code but may need small adjustments on first run.
