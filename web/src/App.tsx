import { SignInModal } from "@coinbase/cdp-react";
import { useEvmAddress, useIsSignedIn } from "@coinbase/cdp-hooks";

export default function App() {
  const { isSignedIn } = useIsSignedIn();
  const { evmAddress } = useEvmAddress();
  return (
    <main style={{ padding: 24, fontFamily: "sans-serif" }}>
      <h1>Bitcoin Wallet</h1>
      {isSignedIn ? (
        <>
          <p>You are signed in.</p>
          <p>
            Your wallet address:{" "}
            <code>{evmAddress ?? "creating your wallet..."}</code>
          </p>
        </>
      ) : (
        <SignInModal />
      )}
    </main>
  );
}
