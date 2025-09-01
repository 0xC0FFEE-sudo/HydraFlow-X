'use client';

import { forwardRef } from 'react';
import { cva, type VariantProps } from 'class-variance-authority';
import { cn } from '@/lib/utils';

const badgeVariants = cva(
  'inline-flex items-center gap-1.5 rounded-full border px-2.5 py-0.5 text-xs font-medium transition-all duration-200',
  {
    variants: {
      variant: {
        default: 'border-primary-500/20 bg-primary-500/10 text-primary-300',
        secondary: 'border-secondary-500/20 bg-secondary-500/10 text-secondary-300',
        success: 'border-success-500/20 bg-success-500/10 text-success-300',
        warning: 'border-warning-500/20 bg-warning-500/10 text-warning-300',
        danger: 'border-danger-500/20 bg-danger-500/10 text-danger-300',
        info: 'border-primary-500/20 bg-primary-500/10 text-primary-300',
        outline: 'border-white/20 bg-transparent text-white/80',
        solid: {
          default: 'border-primary-500 bg-primary-500 text-white',
          secondary: 'border-secondary-500 bg-secondary-500 text-white',
          success: 'border-success-500 bg-success-500 text-white',
          warning: 'border-warning-500 bg-warning-500 text-white',
          danger: 'border-danger-500 bg-danger-500 text-white',
        },
        glow: {
          default: 'border-primary-500/40 bg-primary-500/20 text-primary-200 shadow-glow-sm',
          success: 'border-success-500/40 bg-success-500/20 text-success-200 shadow-sm shadow-success-500/20',
          warning: 'border-warning-500/40 bg-warning-500/20 text-warning-200 shadow-sm shadow-warning-500/20',
          danger: 'border-danger-500/40 bg-danger-500/20 text-danger-200 shadow-sm shadow-danger-500/20',
        },
      },
      size: {
        sm: 'px-2 py-0.5 text-2xs',
        default: 'px-2.5 py-0.5 text-xs',
        lg: 'px-3 py-1 text-sm',
      },
      pulse: {
        true: 'animate-pulse-soft',
        false: '',
      },
    },
    defaultVariants: {
      variant: 'default',
      size: 'default',
      pulse: false,
    },
  }
);

export interface BadgeProps
  extends React.HTMLAttributes<HTMLDivElement>,
    VariantProps<typeof badgeVariants> {
  icon?: React.ReactNode;
  dot?: boolean;
}

export const Badge = forwardRef<HTMLDivElement, BadgeProps>(
  ({ className, variant, size, pulse, icon, dot, children, ...props }, ref) => (
    <div
      ref={ref}
      className={cn(badgeVariants({ variant, size, pulse }), className)}
      {...props}
    >
      {dot && (
        <span className="h-1.5 w-1.5 rounded-full bg-current animate-pulse" />
      )}
      {icon && <span className="h-3 w-3">{icon}</span>}
      {children}
    </div>
  )
);

Badge.displayName = 'Badge';

// Status-specific badge components for common use cases
export const StatusBadge = ({ 
  status, 
  ...props 
}: { 
  status: 'healthy' | 'warning' | 'error' | 'offline' | 'connected' | 'disconnected';
} & Omit<BadgeProps, 'variant'>) => {
  const variants = {
    healthy: 'success',
    connected: 'success', 
    warning: 'warning',
    error: 'danger',
    offline: 'secondary',
    disconnected: 'danger',
  } as const;

  const labels = {
    healthy: 'Healthy',
    connected: 'Connected',
    warning: 'Warning', 
    error: 'Error',
    offline: 'Offline',
    disconnected: 'Disconnected',
  };

  return (
    <Badge 
      variant={variants[status]} 
      dot={status === 'healthy' || status === 'connected'}
      pulse={status === 'warning' || status === 'error'}
      {...props}
    >
      {labels[status]}
    </Badge>
  );
};

export const PnLBadge = ({ 
  value, 
  showSign = true,
  ...props 
}: { 
  value: number;
  showSign?: boolean;
} & Omit<BadgeProps, 'variant' | 'children'>) => {
  const isPositive = value > 0;
  const isNeutral = value === 0;
  
  return (
    <Badge
      variant={isNeutral ? 'secondary' : isPositive ? 'success' : 'danger'}
      {...props}
    >
      {showSign && !isNeutral && (isPositive ? '+' : '')}
      {value.toFixed(2)}%
    </Badge>
  );
};
