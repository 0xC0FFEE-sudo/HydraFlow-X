'use client';

import { useState } from 'react';
import { motion } from 'framer-motion';
import { cn } from '@/lib/utils';
import { Badge, StatusBadge } from '@/components/ui/Badge';
import { Button } from '@/components/ui/Button';
import { 
  LayoutDashboard,
  TrendingUp,
  Wallet,
  Shield,
  Settings,
  Activity,
  BarChart3,
  Zap,
  X,
  ChevronLeft,
  User,
  LogOut,
  HelpCircle,
} from 'lucide-react';

interface SidebarProps {
  activeView: string;
  onViewChange: (view: string) => void;
  onClose?: () => void;
}

const navigationItems = [
  { 
    id: 'overview', 
    name: 'Overview', 
    icon: LayoutDashboard,
    badge: null,
    description: 'System overview and key metrics'
  },
  { 
    id: 'trading', 
    name: 'Trading', 
    icon: TrendingUp,
    badge: { text: '7', variant: 'success' as const },
    description: 'Active strategies and positions'
  },
  { 
    id: 'analytics', 
    name: 'Analytics', 
    icon: BarChart3,
    badge: null,
    description: 'Performance analysis and insights'
  },
  { 
    id: 'mev-protection', 
    name: 'MEV Shield', 
    icon: Shield,
    badge: { text: '97%', variant: 'success' as const },
    description: 'MEV protection and bundle status'
  },
  { 
    id: 'wallets', 
    name: 'Wallets', 
    icon: Wallet,
    badge: { text: '12', variant: 'default' as const },
    description: 'Wallet management and balances'
  },
  { 
    id: 'monitoring', 
    name: 'Monitoring', 
    icon: Activity,
    badge: null,
    description: 'System health and alerts'
  },
  { 
    id: 'settings', 
    name: 'Settings', 
    icon: Settings,
    badge: null,
    description: 'Configuration and preferences'
  },
];

export function Sidebar({ activeView, onViewChange, onClose }: SidebarProps) {
  const [collapsed, setCollapsed] = useState(false);

  return (
    <motion.div 
      className={cn(
        'flex flex-col h-full bg-secondary-900/90 backdrop-blur-xl border-r border-white/10',
        'w-70 transition-all duration-300',
        collapsed && 'w-16'
      )}
      initial={{ x: -20, opacity: 0 }}
      animate={{ x: 0, opacity: 1 }}
      transition={{ duration: 0.3 }}
    >
      {/* Header */}
      <div className="flex items-center justify-between p-4 border-b border-white/10">
        {!collapsed && (
          <motion.div 
            className="flex items-center space-x-3"
            initial={{ opacity: 0 }}
            animate={{ opacity: 1 }}
            transition={{ delay: 0.1 }}
          >
            <div className="relative">
              <div className="w-8 h-8 bg-gradient-to-r from-primary-500 to-accent-500 rounded-lg flex items-center justify-center">
                <Zap className="w-4 h-4 text-white" />
              </div>
              <div className="absolute -top-1 -right-1 w-3 h-3 bg-success-500 rounded-full animate-pulse" />
            </div>
            <div>
              <h1 className="font-bold text-white text-lg">HydraFlow-X</h1>
              <p className="text-xs text-secondary-400">Ultra-Low Latency</p>
            </div>
          </motion.div>
        )}

        <div className="flex items-center space-x-1">
          <Button
            variant="ghost"
            size="icon-sm"
            onClick={() => setCollapsed(!collapsed)}
            className="text-secondary-400 hover:text-white"
          >
            <ChevronLeft className={cn(
              'w-4 h-4 transition-transform duration-200',
              collapsed && 'rotate-180'
            )} />
          </Button>
          
          {onClose && (
            <Button
              variant="ghost"
              size="icon-sm"
              onClick={onClose}
              className="text-secondary-400 hover:text-white lg:hidden"
            >
              <X className="w-4 h-4" />
            </Button>
          )}
        </div>
      </div>

      {/* System Status */}
      {!collapsed && (
        <motion.div 
          className="px-4 py-3 border-b border-white/10"
          initial={{ opacity: 0, y: 10 }}
          animate={{ opacity: 1, y: 0 }}
          transition={{ delay: 0.2 }}
        >
          <div className="flex items-center justify-between mb-2">
            <span className="text-sm font-medium text-white">System Status</span>
            <StatusBadge status="healthy" />
          </div>
          <div className="grid grid-cols-2 gap-2 text-xs">
            <div className="text-secondary-400">
              <span>Latency: </span>
              <span className="text-success-400 font-medium">12.5ms</span>
            </div>
            <div className="text-secondary-400">
              <span>Uptime: </span>
              <span className="text-success-400 font-medium">99.98%</span>
            </div>
          </div>
        </motion.div>
      )}

      {/* Navigation */}
      <nav className="flex-1 px-2 py-4 space-y-1 overflow-y-auto custom-scrollbar">
        {navigationItems.map((item, index) => {
          const Icon = item.icon;
          const isActive = activeView === item.id;
          
          return (
            <motion.div
              key={item.id}
              initial={{ opacity: 0, x: -10 }}
              animate={{ opacity: 1, x: 0 }}
              transition={{ delay: 0.1 + index * 0.05 }}
            >
              <button
                onClick={() => onViewChange(item.id)}
                className={cn(
                  'w-full flex items-center gap-3 px-3 py-2.5 text-sm font-medium rounded-xl transition-all duration-200 group relative',
                  'hover:bg-white/10 focus:outline-none focus:bg-white/10',
                  isActive ? 
                    'bg-primary-600/20 text-primary-300 border border-primary-600/30 shadow-lg shadow-primary-500/10' : 
                    'text-secondary-400 hover:text-white',
                  collapsed && 'justify-center px-2'
                )}
                title={collapsed ? item.name : undefined}
              >
                {/* Active indicator */}
                {isActive && (
                  <motion.div
                    className="absolute left-0 top-1/2 -translate-y-1/2 w-1 h-8 bg-primary-500 rounded-r-full"
                    layoutId="activeIndicator"
                    transition={{ type: "spring", stiffness: 300, damping: 30 }}
                  />
                )}

                {/* Icon */}
                <Icon className={cn(
                  'w-5 h-5 transition-colors duration-200',
                  isActive ? 'text-primary-400' : 'text-secondary-400 group-hover:text-white',
                  collapsed && 'w-6 h-6'
                )} />

                {/* Label and Badge */}
                {!collapsed && (
                  <>
                    <span className="flex-1 text-left">{item.name}</span>
                    {item.badge && (
                      <Badge variant={item.badge.variant} size="sm">
                        {item.badge.text}
                      </Badge>
                    )}
                  </>
                )}

                {/* Tooltip for collapsed mode */}
                {collapsed && (
                  <div className="absolute left-full ml-2 px-3 py-2 bg-secondary-800 text-white text-sm rounded-lg opacity-0 invisible group-hover:opacity-100 group-hover:visible transition-all duration-200 z-50 whitespace-nowrap">
                    {item.name}
                    {item.badge && (
                      <Badge variant={item.badge.variant} size="sm" className="ml-2">
                        {item.badge.text}
                      </Badge>
                    )}
                  </div>
                )}
              </button>
            </motion.div>
          );
        })}
      </nav>

      {/* User section */}
      {!collapsed && (
        <motion.div 
          className="p-4 border-t border-white/10"
          initial={{ opacity: 0, y: 10 }}
          animate={{ opacity: 1, y: 0 }}
          transition={{ delay: 0.3 }}
        >
          <div className="flex items-center space-x-3 mb-3">
            <div className="w-8 h-8 bg-gradient-to-r from-primary-500 to-accent-500 rounded-full flex items-center justify-center">
              <User className="w-4 h-4 text-white" />
            </div>
            <div className="flex-1 min-w-0">
              <p className="text-sm font-medium text-white truncate">Trading User</p>
              <p className="text-xs text-secondary-400">Pro Trader</p>
            </div>
          </div>
          
          <div className="flex space-x-1">
            <Button variant="ghost" size="icon-sm" className="text-secondary-400 hover:text-white">
              <HelpCircle className="w-4 h-4" />
            </Button>
            <Button variant="ghost" size="icon-sm" className="text-secondary-400 hover:text-white">
              <LogOut className="w-4 h-4" />
            </Button>
          </div>
        </motion.div>
      )}
    </motion.div>
  );
}
