'use client';

import { useEffect, useState } from 'react';
import { Sidebar } from '@/components/layout/Sidebar';
import { Header } from '@/components/layout/Header';
import { DashboardRouter } from '@/components/dashboard/DashboardContent';
import { WebSocketProvider } from '@/providers/WebSocketProvider';
import { motion, AnimatePresence } from 'framer-motion';

function DashboardContent() {
  const [activeView, setActiveView] = useState<string>('overview');
  const [sidebarOpen, setSidebarOpen] = useState(true);
  const [isLoading, setIsLoading] = useState(true);

  // Initialize dashboard with smooth loading
  useEffect(() => {
    const initDashboard = async () => {
      try {
        // Simulate initialization time for smooth UX
        await new Promise(resolve => setTimeout(resolve, 1200));
      } catch (error) {
        console.warn('Dashboard initialization error:', error);
      } finally {
        setIsLoading(false);
      }
    };

    initDashboard();
  }, []);

  if (isLoading) {
    return <LoadingScreen />;
  }

  return (
    <div className="flex h-screen bg-secondary-950 overflow-hidden">
      {/* Sidebar */}
      <AnimatePresence mode="wait">
        {sidebarOpen && (
          <motion.div
            initial={{ x: -280, opacity: 0 }}
            animate={{ x: 0, opacity: 1 }}
            exit={{ x: -280, opacity: 0 }}
            transition={{ duration: 0.3, ease: 'easeInOut' }}
            className="fixed inset-y-0 left-0 z-50 w-70 lg:relative lg:w-70"
          >
            <Sidebar
              activeView={activeView}
              onViewChange={setActiveView}
              onClose={() => setSidebarOpen(false)}
            />
          </motion.div>
        )}
      </AnimatePresence>

      {/* Main content area */}
      <div className="flex-1 flex flex-col min-w-0">
        {/* Header */}
        <Header
          onMenuClick={() => setSidebarOpen(!sidebarOpen)}
          sidebarOpen={sidebarOpen}
        />

        {/* Dashboard content */}
        <main className="flex-1 overflow-auto bg-gradient-to-br from-secondary-950 via-secondary-900 to-secondary-950">
          <div className="h-full p-6">
            <motion.div
              key={activeView}
              initial={{ opacity: 0, y: 20 }}
              animate={{ opacity: 1, y: 0 }}
              transition={{ duration: 0.4, ease: 'easeOut' }}
              className="h-full"
            >
              <DashboardRouter activeView={activeView} />
            </motion.div>
          </div>
        </main>
      </div>

      {/* Mobile overlay */}
      {sidebarOpen && (
        <div
          className="fixed inset-0 bg-black/50 backdrop-blur-sm z-40 lg:hidden"
          onClick={() => setSidebarOpen(false)}
        />
      )}
    </div>
  );
}

// Main export wrapped in WebSocket provider
export default function Dashboard() {
  return (
    <WebSocketProvider>
      <DashboardContent />
    </WebSocketProvider>
  );
}

// Beautiful loading screen
function LoadingScreen() {
  return (
    <div className="flex items-center justify-center h-screen bg-secondary-950">
      <div className="text-center space-y-8">
        {/* Animated logo/brand */}
        <motion.div
          initial={{ scale: 0.8, opacity: 0 }}
          animate={{ scale: 1, opacity: 1 }}
          transition={{ duration: 0.6, ease: 'easeOut' }}
          className="relative"
        >
          <div className="w-20 h-20 mx-auto relative">
            <div className="absolute inset-0 bg-gradient-to-r from-primary-500 to-accent-500 rounded-2xl animate-pulse" />
            <div className="absolute inset-2 bg-secondary-950 rounded-xl flex items-center justify-center">
              <span className="text-2xl font-bold gradient-text">H</span>
            </div>
          </div>
        </motion.div>

        {/* Title */}
        <motion.div
          initial={{ opacity: 0, y: 20 }}
          animate={{ opacity: 1, y: 0 }}
          transition={{ duration: 0.6, delay: 0.2, ease: 'easeOut' }}
          className="space-y-2"
        >
          <h1 className="text-3xl font-bold gradient-text">
            HydraFlow-X
          </h1>
          <p className="text-secondary-400 text-lg">
            Ultra-Low Latency Trading
          </p>
        </motion.div>

        {/* Loading animation */}
        <motion.div
          initial={{ opacity: 0 }}
          animate={{ opacity: 1 }}
          transition={{ duration: 0.6, delay: 0.4 }}
          className="flex justify-center space-x-2"
        >
          {[0, 1, 2].map((i) => (
            <motion.div
              key={i}
              className="w-2 h-2 bg-primary-500 rounded-full"
              animate={{ y: [-4, 4, -4] }}
              transition={{ 
                duration: 1,
                delay: i * 0.2,
                repeat: Infinity,
                ease: 'easeInOut'
              }}
            />
          ))}
        </motion.div>

        {/* Status text */}
        <motion.p
          initial={{ opacity: 0 }}
          animate={{ opacity: 1 }}
          transition={{ duration: 0.6, delay: 0.6 }}
          className="text-secondary-500 text-sm"
        >
          Initializing trading systems<span className="loading-dots">...</span>
        </motion.p>
      </div>
    </div>
  );
}