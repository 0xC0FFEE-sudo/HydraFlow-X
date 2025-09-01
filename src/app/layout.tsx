import React from 'react';
import type { Metadata } from 'next';
import { Inter } from 'next/font/google';
import './globals.css';
import { Toaster } from 'react-hot-toast';

const inter = Inter({ 
  subsets: ['latin'],
  display: 'swap',
  variable: '--font-inter',
});

export const metadata: Metadata = {
  title: 'HydraFlow-X | Ultra-Low Latency Trading Dashboard',
  description: 'Advanced algorithmic trading platform with MEV protection, real-time analytics, and ultra-low latency execution.',
  keywords: ['trading', 'algorithmic', 'MEV', 'DeFi', 'cryptocurrency', 'low-latency', 'arbitrage'],
  authors: [{ name: 'HydraFlow Team' }],
  viewport: 'width=device-width, initial-scale=1',
  themeColor: '#0ea5e9',
  robots: 'noindex, nofollow', // Private trading dashboard
};

export default function RootLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  return (
    <html lang="en" className="dark">
      <head>
        <link rel="icon" href="/favicon.svg" type="image/svg+xml" />
        <link rel="apple-touch-icon" href="/apple-touch-icon.png" />
        <meta name="theme-color" content="#0ea5e9" />
      </head>
      <body 
        className={`${inter.variable} font-sans antialiased bg-secondary-950 text-white overflow-hidden`}
        suppressHydrationWarning
      >
        {/* Background with gradient and subtle pattern */}
        <div className="fixed inset-0 bg-gradient-to-br from-secondary-950 via-secondary-900 to-secondary-950">
          <div className="absolute inset-0 bg-[radial-gradient(ellipse_at_top,_var(--tw-gradient-stops))] from-primary-900/20 via-transparent to-transparent" />
          <div className="absolute inset-0 bg-grid-white/[0.02] bg-[size:60px_60px]" />
        </div>

        {/* Main content */}
        <div className="relative z-10 h-screen">
          {children}
        </div>

        {/* Toast notifications */}
        <Toaster
          position="top-right"
          toastOptions={{
            duration: 4000,
            style: {
              background: '#1e293b',
              color: '#f8fafc',
              border: '1px solid #334155',
              borderRadius: '12px',
              fontSize: '14px',
              backdropFilter: 'blur(12px)',
            },
            success: {
              iconTheme: {
                primary: '#10b981',
                secondary: '#ffffff',
              },
            },
            error: {
              iconTheme: {
                primary: '#ef4444', 
                secondary: '#ffffff',
              },
            },
            loading: {
              iconTheme: {
                primary: '#0ea5e9',
                secondary: '#ffffff',
              },
            },
          }}
        />

        {/* Global styles for custom scrollbars and animations */}
        <style>{`
          .bg-grid-white {
            background-image: radial-gradient(circle at 1px 1px, rgba(255,255,255,0.15) 1px, transparent 0);
          }
          
          /* Custom scrollbar styling */
          ::-webkit-scrollbar {
            width: 6px;
            height: 6px;
          }
          
          ::-webkit-scrollbar-track {
            background: rgba(30, 41, 59, 0.3);
            border-radius: 3px;
          }
          
          ::-webkit-scrollbar-thumb {
            background: rgba(100, 116, 139, 0.5);
            border-radius: 3px;
          }
          
          ::-webkit-scrollbar-thumb:hover {
            background: rgba(100, 116, 139, 0.7);
          }
          
          /* Selection styling */
          ::selection {
            background-color: rgba(14, 165, 233, 0.3);
            color: inherit;
          }

          /* Smooth focus transitions */
          *:focus-visible {
            transition: all 0.2s ease;
          }
        `}</style>
      </body>
    </html>
  );
}