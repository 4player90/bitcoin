import { CDPReactProvider, type Config } from "@coinbase/cdp-react";
import { StrictMode } from "react";
import { createRoot } from "react-dom/client";

import App from "./App";

const config: Config = {
  projectId: import.meta.env.VITE_CDP_PROJECT_ID ?? "your-cdp-project-id",
  appName: "Bitcoin Wallet",
  ethereum: { createOnLogin: "smart" },
  authMethods: ["email", "sms", "oauth:x", "siwe:base"],
};

createRoot(document.getElementById("root")!).render(
  <StrictMode>
    <CDPReactProvider config={config}>
      <App />
    </CDPReactProvider>
  </StrictMode>,
);
