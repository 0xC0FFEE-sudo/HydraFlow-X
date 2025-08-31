'use client';

import { Inter } from 'next/font/google';
import './globals.css';
import { Toaster } from 'react-hot-toast';
import { WebSocketProvider } from '@/providers/WebSocketProvider';
import { ConfigProvider } from '@/providers/ConfigProvider';

const inter = Inter({ subsets: ['latin'] });

export default function RootLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  return (
    <html lang="en" className="dark">
      <body className={`${inter.className} bg-gray-900 text-gray-100`}>
        <ConfigProvider>
          <WebSocketProvider>
            <div className="min-h-screen bg-gradient-to-br from-gray-900 via-blue-900 to-purple-900">
              <main className="relative z-10">
                {children}
              </main>
              <Toaster
                position="top-right"
                toastOptions={{
                  duration: 4000,
                  style: {
                    background: '#1f2937',
                    color: '#f3f4f6',
                    border: '1px solid #374151',
                  },
                  success: {
                    iconTheme: {
                      primary: '#10b981',
                      secondary: '#f3f4f6',
                    },
                  },
                  error: {
                    iconTheme: {
                      primary: '#ef4444',
                      secondary: '#f3f4f6',
                    },
                  },
                }}
              />
            </div>
          </WebSocketProvider>
        </ConfigProvider>
      </body>
    </html>
  );
}
