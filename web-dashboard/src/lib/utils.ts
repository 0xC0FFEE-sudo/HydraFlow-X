import { type ClassValue, clsx } from "clsx"
import { twMerge } from "tailwind-merge"

export function cn(...inputs: ClassValue[]) {
  return twMerge(clsx(inputs))
}

export function formatCurrency(value: number): string {
  return new Intl.NumberFormat('en-US', {
    style: 'currency',
    currency: 'USD',
  }).format(value)
}

export function formatPercentage(value: number): string {
  return new Intl.NumberFormat('en-US', {
    style: 'percent',
    minimumFractionDigits: 2,
    maximumFractionDigits: 2,
  }).format(value / 100)
}

export function formatLatency(nanoseconds: number): string {
  if (nanoseconds >= 1000000) {
    return `${(nanoseconds / 1000000).toFixed(2)}ms`
  } else if (nanoseconds >= 1000) {
    return `${(nanoseconds / 1000).toFixed(2)}μs`
  } else {
    return `${nanoseconds.toFixed(0)}ns`
  }
}
