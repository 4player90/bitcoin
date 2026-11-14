import { Address, Chain, TypedDataDomain } from 'viem'

export const eip2612Abi = [
  {
    constant: false,
    inputs: [
      { name: 'owner', type: 'address' },
      { name: 'spender', type: 'address' },
      { name: 'value', type: 'uint256' },
      { name: 'deadline', type: 'uint256' },
      { name: 'v', type: 'uint8' },
      { name: 'r', type: 'bytes32' },
      { name: 's', type: 'bytes32' },
    ],
    name: 'permit',
    outputs: [],
    payable: false,
    stateMutability: 'nonpayable',
    type: 'function',
  },
] as const

export const tokenAbi = [
  ...eip2612Abi,
  {
    inputs: [{ name: 'owner', type: 'address' }],
    name: 'nonces',
    outputs: [{ name: '', type: 'uint256' }],
    stateMutability: 'view',
    type: 'function',
  },
  {
    inputs: [],
    name: 'name',
    outputs: [{ name: '', type: 'string' }],
    stateMutability: 'view',
    type: 'function',
  },
  {
    inputs: [],
    name: 'version',
    outputs: [{ name: '', type: 'string' }],
    stateMutability: 'view',
    type: 'function',
  },
  {
    inputs: [
      { name: 'recipient', type: 'address' },
      { name: 'amount', type: 'uint256' },
    ],
    name: 'transfer',
    outputs: [{ name: '', type: 'bool' }],
    stateMutability: 'nonpayable',
    type: 'function',
  },
  {
    inputs: [{ name: 'account', type: 'address' }],
    name: 'balanceOf',
    outputs: [{ name: '', type: 'uint256' }],
    stateMutability: 'view',
    type: 'function',
  },
] as const

export async function eip2612Permit({
  token,
  chain,
  ownerAddress,
  spenderAddress,
  value,
}: {
  token: {
    address: Address
    read: {
      nonces: (args: [Address]) => Promise<bigint>
      name: () => Promise<string>
      version: () => Promise<string>
    }
  }
  chain: Chain
  ownerAddress: Address
  spenderAddress: Address
  value: bigint
}) {
  const [nonce, name, version] = await Promise.all([
    token.read.nonces([ownerAddress]),
    token.read.name(),
    token.read.version(),
  ])

  const domain: TypedDataDomain = {
    name,
    version,
    chainId: chain.id,
    verifyingContract: token.address,
  }

  const types = {
    Permit: [
      { name: 'owner', type: 'address' },
      { name: 'spender', type: 'address' },
      { name: 'value', type: 'uint256' },
      { name: 'nonce', type: 'uint256' },
      { name: 'deadline', type: 'uint256' },
    ],
  }

  const message = {
    owner: ownerAddress,
    spender: spenderAddress,
    value,
    nonce,
    deadline: BigInt('0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff'),
  }

  return {
    domain,
    types,
    message,
  }
}
