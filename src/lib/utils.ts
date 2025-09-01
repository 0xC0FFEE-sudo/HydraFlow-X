import { clsx, type ClassValue } from 'clsx';
import { twMerge } from 'tailwind-merge';

// Tailwind class merger utility
export function cn(...inputs: ClassValue[]) {
  return twMerge(clsx(inputs));
}

// Number formatting utilities
export const formatters = {
  // Currency formatting
  currency: (value: number, decimals: number = 2) => {
    return new Intl.NumberFormat('en-US', {
      style: 'currency',
      currency: 'USD',
      minimumFractionDigits: decimals,
      maximumFractionDigits: decimals,
    }).format(value);
  },

  // Percentage formatting
  percentage: (value: number, decimals: number = 2) => {
    return new Intl.NumberFormat('en-US', {
      style: 'percent',
      minimumFractionDigits: decimals,
      maximumFractionDigits: decimals,
    }).format(value / 100);
  },

  // Number with thousand separators
  number: (value: number, decimals: number = 2) => {
    return new Intl.NumberFormat('en-US', {
      minimumFractionDigits: decimals,
      maximumFractionDigits: decimals,
    }).format(value);
  },

  // Compact number formatting (1.2K, 3.4M, etc.)
  compact: (value: number) => {
    return new Intl.NumberFormat('en-US', {
      notation: 'compact',
      maximumFractionDigits: 1,
    }).format(value);
  },

  // Latency formatting (microseconds to readable format)
  latency: (microseconds: number) => {
    if (microseconds < 1000) {
      return `${microseconds.toFixed(1)}μs`;
    } else if (microseconds < 1000000) {
      return `${(microseconds / 1000).toFixed(1)}ms`;
    } else {
      return `${(microseconds / 1000000).toFixed(2)}s`;
    }
  },

  // Bytes formatting
  bytes: (bytes: number) => {
    const units = ['B', 'KB', 'MB', 'GB', 'TB'];
    let size = bytes;
    let unitIndex = 0;

    while (size >= 1024 && unitIndex < units.length - 1) {
      size /= 1024;
      unitIndex++;
    }

    return `${size.toFixed(1)} ${units[unitIndex]}`;
  },

  // Time formatting
  timeAgo: (timestamp: number) => {
    const now = Date.now();
    const diff = now - timestamp;
    const seconds = Math.floor(diff / 1000);
    const minutes = Math.floor(seconds / 60);
    const hours = Math.floor(minutes / 60);
    const days = Math.floor(hours / 24);

    if (days > 0) return `${days}d ago`;
    if (hours > 0) return `${hours}h ago`;
    if (minutes > 0) return `${minutes}m ago`;
    return `${seconds}s ago`;
  },

  // Duration formatting
  duration: (seconds: number) => {
    const days = Math.floor(seconds / 86400);
    const hours = Math.floor((seconds % 86400) / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    const secs = seconds % 60;

    if (days > 0) return `${days}d ${hours}h ${minutes}m`;
    if (hours > 0) return `${hours}h ${minutes}m`;
    if (minutes > 0) return `${minutes}m ${Math.floor(secs)}s`;
    return `${Math.floor(secs)}s`;
  },
};

// Color utilities for charts and indicators
export const colors = {
  // PnL colors
  pnl: (value: number) => {
    if (value > 0) return 'text-success-500';
    if (value < 0) return 'text-danger-500';
    return 'text-secondary-400';
  },

  // Status colors
  status: {
    healthy: 'text-success-500',
    warning: 'text-warning-500',
    error: 'text-danger-500',
    info: 'text-primary-500',
  },

  // Trading side colors
  side: {
    buy: 'text-success-500',
    sell: 'text-danger-500',
    long: 'text-success-500',
    short: 'text-danger-500',
  },

  // Background variants
  bg: {
    success: 'bg-success-500/10 border-success-500/20',
    warning: 'bg-warning-500/10 border-warning-500/20',
    danger: 'bg-danger-500/10 border-danger-500/20',
    info: 'bg-primary-500/10 border-primary-500/20',
  },
};

// Chart color palettes
export const chartColors = {
  primary: ['#0ea5e9', '#06b6d4', '#10b981', '#f59e0b', '#ef4444'],
  gradient: {
    blue: ['#0ea5e9', '#0284c7'],
    green: ['#10b981', '#059669'],
    red: ['#ef4444', '#dc2626'],
    purple: ['#8b5cf6', '#7c3aed'],
  },
};

// Calculation utilities
export const calculations = {
  // Calculate PnL percentage
  pnlPercent: (current: number, entry: number) => {
    return ((current - entry) / entry) * 100;
  },

  // Calculate average
  average: (values: number[]) => {
    return values.length > 0 ? values.reduce((sum, val) => sum + val, 0) / values.length : 0;
  },

  // Calculate percentile
  percentile: (values: number[], p: number) => {
    const sorted = [...values].sort((a, b) => a - b);
    const index = (p / 100) * (sorted.length - 1);
    const floor = Math.floor(index);
    const ceil = Math.ceil(index);
    
    if (floor === ceil) return sorted[floor];
    return sorted[floor] * (ceil - index) + sorted[ceil] * (index - floor);
  },

  // Calculate standard deviation
  standardDeviation: (values: number[]) => {
    const avg = calculations.average(values);
    const squareDiffs = values.map(value => Math.pow(value - avg, 2));
    const avgSquareDiff = calculations.average(squareDiffs);
    return Math.sqrt(avgSquareDiff);
  },

  // Risk calculations
  sharpeRatio: (returns: number[], riskFreeRate: number = 0) => {
    const avgReturn = calculations.average(returns);
    const stdDev = calculations.standardDeviation(returns);
    return stdDev === 0 ? 0 : (avgReturn - riskFreeRate) / stdDev;
  },
};

// Validation utilities
export const validators = {
  isValidAddress: (address: string) => {
    return /^0x[a-fA-F0-9]{40}$/.test(address);
  },

  isPositiveNumber: (value: string | number) => {
    const num = typeof value === 'string' ? parseFloat(value) : value;
    return !isNaN(num) && num > 0;
  },

  isValidSymbol: (symbol: string) => {
    return /^[A-Z0-9]+\/[A-Z0-9]+$/.test(symbol);
  },
};

// Local storage utilities with error handling
export const storage = {
  get: <T>(key: string, defaultValue: T): T => {
    try {
      const item = localStorage.getItem(key);
      return item ? JSON.parse(item) : defaultValue;
    } catch {
      return defaultValue;
    }
  },

  set: <T>(key: string, value: T): void => {
    try {
      localStorage.setItem(key, JSON.stringify(value));
    } catch (error) {
      console.warn('Failed to save to localStorage:', error);
    }
  },

  remove: (key: string): void => {
    try {
      localStorage.removeItem(key);
    } catch (error) {
      console.warn('Failed to remove from localStorage:', error);
    }
  },
};
