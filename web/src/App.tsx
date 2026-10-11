import { SignInModal } from "@coinbase/cdp-react";
import { useIsSignedIn } from "@coinbase/cdp-hooks";

export default function App() {
  const { isSignedIn } = useIsSignedIn();
  return (
    <main style={{ padding: 24, fontFamily: "sans-serif" }}>
      <h1>Bitcoin Wallet</h1>
      {isSignedIn ? <p>You are signed in.</p> : <SignInModal />}
    </main>
  );
}
