const { expect } = require("chai");
const { ethers, network } = require("hardhat");

// Order layout: userId (8 bytes) | buyAmount (12 bytes) | sellAmount (12 bytes)
const encodeOrder = (userId, buy, sell) =>
  ethers.utils.hexZeroPad(
    ethers.BigNumber.from(userId)
      .shl(192)
      .add(ethers.BigNumber.from(buy).shl(96))
      .add(ethers.BigNumber.from(sell))
      .toHexString(),
    32
  );
const QUEUE_START = ethers.utils.hexZeroPad("0x01", 32);

const increaseTime = async (s) => {
  await network.provider.send("evm_increaseTime", [s]);
  await network.provider.send("evm_mine");
};

describe("EasyAuction", () => {
  let owner, factory, bidder, other, auction, auctioning, bidding, now;
  const AUCTIONED = 1000;
  const MIN_BUY = 1000; // bidding tokens the auctioneer wants at least
  const DAY = 86400;

  beforeEach(async () => {
    [owner, factory, bidder, other] = await ethers.getSigners();
    const Token = await ethers.getContractFactory("MockERC20");
    auctioning = await Token.deploy("Auctioning", "AUC");
    bidding = await Token.deploy("Bidding", "BID");
    const EasyAuction = await ethers.getContractFactory("EasyAuction");
    auction = await EasyAuction.deploy();
    await auction.setSubjectFactory(factory.address);

    await auctioning.mint(factory.address, 10000);
    await auctioning.connect(factory).approve(auction.address, 10000);
    await bidding.mint(bidder.address, 100000);
    await bidding.connect(bidder).approve(auction.address, 100000);
    now = (await ethers.provider.getBlock("latest")).timestamp;
  });

  const initiate = (opts = {}) =>
    auction
      .connect(factory)
      .initiateAuction(
        auctioning.address,
        bidding.address,
        now + DAY, // cancellation end
        now + 2 * DAY, // auction end
        AUCTIONED,
        MIN_BUY,
        1, // minimumBiddingAmountPerOrder
        opts.minFunding || 0,
        false,
        ethers.constants.AddressZero,
        "0x"
      );

  describe("access control", () => {
    it("only owner can set the factory", async () => {
      await expect(
        auction.connect(other).setSubjectFactory(other.address)
      ).to.be.reverted;
    });

    it("only the factory can initiate an auction", async () => {
      await expect(
        auction
          .connect(other)
          .initiateAuction(
            auctioning.address, bidding.address, now + DAY, now + 2 * DAY,
            AUCTIONED, MIN_BUY, 1, 0, false, ethers.constants.AddressZero, "0x"
          )
      ).to.be.revertedWith("Caller is not the subject factory address");
    });

    it("anyone can settle after the auction ends, not before", async () => {
      await initiate();
      await expect(auction.connect(other).settleAuction(1)).to.be.revertedWith(
        "Auction not in solution submission phase"
      );
      await increaseTime(2 * DAY + 1);
      await auction.connect(other).settleAuction(1);
      expect((await auction.auctionData(1)).clearingPriceOrder).to.not.equal(
        ethers.constants.HashZero
      );
    });

    it("only the factory can settle atomically", async () => {
      await initiate();
      await increaseTime(2 * DAY + 1);
      await expect(
        auction.connect(other).settleAuctionAtomically(1, [1000], [2000], [QUEUE_START], "0x")
      ).to.be.revertedWith("Caller is not the subject factory address");
    });

    it("fee is capped at 1.5%", async () => {
      await expect(
        auction.setFeeParameters(16, owner.address)
      ).to.be.revertedWith("Fee is not allowed to be set higher than 1.5%");
    });
  });

  describe("initiateAuction", () => {
    it("escrows the auctioned amount plus fee", async () => {
      await auction.setFeeParameters(10, owner.address); // 1%
      await initiate();
      expect(await auctioning.balanceOf(auction.address)).to.equal(1010);
    });

    it("rejects an end date in the past", async () => {
      await expect(
        auction
          .connect(factory)
          .initiateAuction(
            auctioning.address, bidding.address, now - 10, now - 5,
            AUCTIONED, MIN_BUY, 1, 0, false, ethers.constants.AddressZero, "0x"
          )
      ).to.be.revertedWith("auction end date must be in the future");
    });
  });

  describe("placing and cancelling orders", () => {
    beforeEach(initiate);

    it("rejects an order whose price is not better than the minimum", async () => {
      await expect(
        auction.connect(bidder).placeSellOrders(1, [1000], [1000], [QUEUE_START], "0x")
      ).to.be.revertedWith("limit price not better than mimimal offer");
    });

    it("accepts a valid order and pulls the bidding tokens", async () => {
      await auction.connect(bidder).placeSellOrders(1, [1000], [2000], [QUEUE_START], "0x");
      expect(await bidding.balanceOf(auction.address)).to.equal(2000);
    });

    it("refunds on cancel before the cancellation deadline", async () => {
      await auction.connect(bidder).placeSellOrders(1, [1000], [2000], [QUEUE_START], "0x");
      const id = await auction.callStatic.getUserId(bidder.address);
      await auction.connect(bidder).cancelSellOrders(1, [encodeOrder(id, 1000, 2000)]);
      expect(await bidding.balanceOf(bidder.address)).to.equal(100000);
    });

    it("blocks cancelling after the deadline", async () => {
      await auction.connect(bidder).placeSellOrders(1, [1000], [2000], [QUEUE_START], "0x");
      const id = await auction.callStatic.getUserId(bidder.address);
      await increaseTime(DAY + 1);
      await expect(
        auction.connect(bidder).cancelSellOrders(1, [encodeOrder(id, 1000, 2000)])
      ).to.be.revertedWith("no longer in order placement and cancelation phase");
    });

    it("blocks placing orders after the auction ends", async () => {
      await increaseTime(2 * DAY + 1);
      await expect(
        auction.connect(bidder).placeSellOrders(1, [1000], [2000], [QUEUE_START], "0x")
      ).to.be.revertedWith("no longer in order placement phase");
    });
  });

  describe("settlement and claims", () => {
    it("settles fully filled auction at the bidder's price", async () => {
      await initiate();
      await auction.connect(bidder).placeSellOrders(1, [1000], [2000], [QUEUE_START], "0x");
      await increaseTime(2 * DAY + 1);
      await auction.connect(factory).settleAuction(1);

      // auctioneer (the factory) receives the bidding tokens
      expect(await bidding.balanceOf(factory.address)).to.equal(2000);

      const id = await auction.callStatic.getUserId(bidder.address);
      await auction.connect(bidder).claimFromParticipantOrder(1, [encodeOrder(id, 1000, 2000)]);
      expect(await auctioning.balanceOf(bidder.address)).to.equal(1000);
    });

    it("cannot be settled twice", async () => {
      await initiate();
      await increaseTime(2 * DAY + 1);
      await auction.connect(factory).settleAuction(1);
      await expect(auction.connect(factory).settleAuction(1)).to.be.revertedWith(
        "Auction not in solution submission phase"
      );
    });

    it("refunds everyone if the minimum funding threshold is missed", async () => {
      await initiate({ minFunding: 5000 });
      await auction.connect(bidder).placeSellOrders(1, [1000], [2000], [QUEUE_START], "0x");
      await increaseTime(2 * DAY + 1);
      await auction.connect(factory).settleAuction(1);

      const id = await auction.callStatic.getUserId(bidder.address);
      await auction.connect(bidder).claimFromParticipantOrder(1, [encodeOrder(id, 1000, 2000)]);
      expect(await bidding.balanceOf(bidder.address)).to.equal(100000);
      expect(await auctioning.balanceOf(factory.address)).to.equal(10000); // returned
    });

    it("an order cannot be claimed twice", async () => {
      await initiate();
      await auction.connect(bidder).placeSellOrders(1, [1000], [2000], [QUEUE_START], "0x");
      await increaseTime(2 * DAY + 1);
      await auction.connect(factory).settleAuction(1);
      const id = await auction.callStatic.getUserId(bidder.address);
      const order = encodeOrder(id, 1000, 2000);
      await auction.connect(bidder).claimFromParticipantOrder(1, [order]);
      await expect(
        auction.connect(bidder).claimFromParticipantOrder(1, [order])
      ).to.be.revertedWith("order is no longer claimable");
    });
  });
});
