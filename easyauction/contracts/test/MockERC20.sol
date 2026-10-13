// SPDX-License-Identifier: LGPL-3.0-or-later
pragma solidity >=0.6.8;

import "@openzeppelin/contracts/token/ERC20/ERC20.sol";

contract MockERC20 is ERC20 {
    constructor(string memory name, string memory symbol)
        public
        ERC20(name, symbol)
    {}

    function mint(address to, uint256 amount) external {
        _mint(to, amount);
    }
}
