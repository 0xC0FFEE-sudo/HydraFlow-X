'use client';

import { clsx } from 'clsx';

interface BadgeProps extends React.HTMLAttributes<HTMLDivElement> {
  variant?: 'default' | 'secondary' | 'destructive' | 'success' | 'warning';
  children: React.ReactNode;
}

export function Badge({ className, variant = 'default', children, ...props }: BadgeProps) {
  return (
    <div
      className={clsx(
        'inline-flex items-center rounded-full px-2.5 py-0.5 text-xs font-medium transition-colors',
        {
          'bg-blue-600 text-blue-100': variant === 'default',
          'bg-gray-600 text-gray-100': variant === 'secondary',
          'bg-red-600 text-red-100': variant === 'destructive',
          'bg-green-600 text-green-100': variant === 'success',
          'bg-yellow-600 text-yellow-100': variant === 'warning',
        },
        className
      )}
      {...props}
    >
      {children}
    </div>
  );
}
