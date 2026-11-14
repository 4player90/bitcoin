'use client'
/* eslint-disable @typescript-eslint/no-explicit-any */

import { useState, useEffect } from 'react'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Button } from '@/components/ui/button'
import { Input } from '@/components/ui/input'
import { Label } from '@/components/ui/label'
import { Tabs, TabsContent, TabsList, TabsTrigger } from '@/components/ui/tabs'
import { Alert, AlertDescription } from '@/components/ui/alert'
import { Loader2 } from 'lucide-react'
import { createPublicClient, http, formatUnits } from 'viem'
import { generatePrivateKey, privateKeyToAccount } from 'viem/accounts'
import { arbitrumSepolia } from 'viem/chains'
import { toEcdsaKernelSmartAccount } from 'permissionless/accounts'
import { tokenAbi } from '@/lib/permit-helpers'
import { transferUSDC } from '@/lib/transfer-service'

const ARBITRUM_SEPOLIA_USDC = '0x75faf114eafb1BDbe2F0316DF893fd58CE46AA4d'

export default function SmartWallet() {
  const [loading, setLoading] = useState(false)
  const [account, setAccount] = useState<any>(null)
  const [recipientAddress, setRecipientAddress] = useState('')
  const [amount, setAmount] = useState('')
  const [status, setStatus] = useState('')
  const [usdcBalance, setUsdcBalance] = useState<string>('0.00')

  useEffect(() => {
    const fetchBalance = async () => {
      if (!account?.address) return

      const client = createPublicClient({
        chain: arbitrumSepolia,
        transport: http(),
      })

      const balance = await client.readContract({
        address: ARBITRUM_SEPOLIA_USDC,
        abi: tokenAbi,
        functionName: 'balanceOf',
        args: [account.address],
      })

      const formattedBalance = Number(formatUnits(balance as bigint, 6)).toFixed(2)
      setUsdcBalance(formattedBalance)
    }

    fetchBalance()
    // Poll every 10 seconds
    const interval = setInterval(fetchBalance, 10000)
    return () => clearInterval(interval)
  }, [account?.address])

  const createAccount = async () => {
    try {
      setLoading(true)
      setStatus('Creating smart account...')

      const client = createPublicClient({
        chain: arbitrumSepolia,
        transport: http(),
      })

      // Generate private key and create owner account
      const privateKey = generatePrivateKey()
      const owner = privateKeyToAccount(privateKey)

      // Create smart account
      const smartAccount = await toEcdsaKernelSmartAccount({
        client,
        owners: [owner],
        version: '0.3.1',
      })

      setAccount({
        address: smartAccount.address,
        owner: owner.address,
        privateKey,
      })

      setStatus('Smart account created successfully!')
    } catch (error) {
      setStatus('Error creating smart account: ' + (error as Error).message)
    } finally {
      setLoading(false)
    }
  }

  const transfer = async () => {
    try {
      setLoading(true)
      setStatus('Checking balance...')

      const client = createPublicClient({
        chain: arbitrumSepolia,
        transport: http(),
      })

      const balance = (await client.readContract({
        address: ARBITRUM_SEPOLIA_USDC,
        abi: tokenAbi,
        functionName: 'balanceOf',
        args: [account.address],
      })) as bigint

      // Convert input amount to USDC decimals (6 decimals)
      const amountInWei = BigInt(Math.floor(parseFloat(amount) * 1_000_000))

      // Required gas buffer (2 USDC to be safe)
      const gasBuffer = BigInt(2_000_000)
      const totalNeeded = amountInWei + gasBuffer

      if (balance < totalNeeded) {
        const currentBalance = Number(formatUnits(balance, 6))
        const requestedAmount = Number(amount)
        const availableForTransfer = Math.max(0, currentBalance - 2)

        throw new Error(
          `Insufficient balance for this transfer. ` +
            `\nCurrent balance: ${currentBalance} USDC` +
            `\nRequested transfer: ${requestedAmount} USDC` +
            `\nGas buffer needed: 2 USDC` +
            `\nMaximum you can transfer: ${availableForTransfer.toFixed(2)} USDC` +
            `\n\nPlease reduce your transfer amount or get more USDC from the faucet.`
        )
      }

      setStatus('Initiating transfer...')

      const receipt = await transferUSDC(account.privateKey, recipientAddress, amountInWei)

      if (receipt.success) {
        setStatus('Transfer completed successfully!')
        setRecipientAddress('')
        setAmount('')
      } else {
        setStatus('Transfer failed. Please try again.')
      }
    } catch (error: any) {
      if (error.message.includes('0x65c8fd4d')) {
        setStatus('Error: Insufficient USDC balance for transfer and gas fees (need ~2 USDC for gas)')
      } else {
        setStatus(error.message)
      }
    } finally {
      setLoading(false)
    }
  }

  return (
    <>
      {account && (
        <div className="fixed top-4 right-4 rounded-lg border bg-white p-3 shadow">
          <span className="text-sm font-medium">USDC Balance: </span>
          <span className="font-mono">${usdcBalance}</span>
        </div>
      )}
      <div className="container max-w-2xl mx-auto p-4">
        <Card>
          <CardHeader>
            <CardTitle>Smart Wallet Interface</CardTitle>
            <CardDescription>
              Create and manage your smart account with Circle&apos;s USDC Paymaster (Arbitrum Sepolia testnet)
            </CardDescription>
          </CardHeader>
          <CardContent>
            <Tabs defaultValue="create" className="space-y-4">
              <TabsList>
                <TabsTrigger value="create">Create Account</TabsTrigger>
                <TabsTrigger value="transfer" disabled={!account}>
                  Transfer
                </TabsTrigger>
              </TabsList>

              <TabsContent value="create" className="space-y-4">
                {!account ? (
                  <Button onClick={createAccount} disabled={loading} className="w-full">
                    {loading && <Loader2 className="mr-2 h-4 w-4 animate-spin" />}
                    Create Smart Account
                  </Button>
                ) : (
                  <div className="space-y-4">
                    <div className="space-y-2">
                      <Label>Smart Wallet Address</Label>
                      <Alert>
                        <AlertDescription className="font-mono break-all">{account.address}</AlertDescription>
                      </Alert>
                    </div>
                    <div className="space-y-2">
                      <Label>Owner Address</Label>
                      <Alert>
                        <AlertDescription className="font-mono break-all">{account.owner}</AlertDescription>
                      </Alert>
                    </div>
                  </div>
                )}
              </TabsContent>

              <TabsContent value="transfer" className="space-y-4">
                <div className="space-y-4">
                  <div className="space-y-2">
                    <Label htmlFor="recipient">Recipient Address</Label>
                    <Input
                      id="recipient"
                      placeholder="0x..."
                      value={recipientAddress}
                      onChange={(e) => setRecipientAddress(e.target.value)}
                    />
                  </div>
                  <div className="space-y-2">
                    <Label htmlFor="amount">Amount (USDC)</Label>
                    <Input
                      id="amount"
                      type="number"
                      placeholder="0.00"
                      value={amount}
                      onChange={(e) => setAmount(e.target.value)}
                    />
                  </div>
                  <Button
                    onClick={transfer}
                    disabled={loading || !recipientAddress || !amount}
                    className="w-full"
                  >
                    {loading && <Loader2 className="mr-2 h-4 w-4 animate-spin" />}
                    Transfer USDC
                  </Button>
                </div>
              </TabsContent>
            </Tabs>

            {status && (
              <Alert className="mt-4">
                <AlertDescription>{status}</AlertDescription>
              </Alert>
            )}
          </CardContent>
        </Card>
      </div>
    </>
  )
}
