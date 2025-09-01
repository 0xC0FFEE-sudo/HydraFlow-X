'use client';

import { forwardRef } from 'react';
import { cva, type VariantProps } from 'class-variance-authority';
import { cn } from '@/lib/utils';

const inputVariants = cva(
  'flex w-full border backdrop-blur-sm transition-all duration-200 focus-visible:outline-none disabled:cursor-not-allowed disabled:opacity-50',
  {
    variants: {
      variant: {
        default: 'bg-secondary-800/50 border-secondary-600 text-white placeholder:text-secondary-400 focus:border-primary-500 focus:bg-secondary-800/80',
        glass: 'bg-white/10 border-white/20 text-white placeholder:text-white/60 focus:border-primary-400 focus:bg-white/20 backdrop-blur-md',
        outline: 'bg-transparent border-white/30 text-white placeholder:text-white/50 focus:border-primary-500 focus:bg-white/5',
        filled: 'bg-secondary-700 border-secondary-600 text-white placeholder:text-secondary-300 focus:border-primary-500',
      },
      size: {
        default: 'h-10 px-3 py-2 text-sm rounded-lg',
        sm: 'h-8 px-2.5 py-1.5 text-xs rounded-md',
        lg: 'h-12 px-4 py-3 text-base rounded-lg',
      },
      state: {
        default: '',
        error: 'border-danger-500 focus:border-danger-400',
        success: 'border-success-500 focus:border-success-400',
        warning: 'border-warning-500 focus:border-warning-400',
      },
    },
    defaultVariants: {
      variant: 'default',
      size: 'default', 
      state: 'default',
    },
  }
);

export interface InputProps
  extends Omit<React.InputHTMLAttributes<HTMLInputElement>, 'size'>,
    VariantProps<typeof inputVariants> {
  leftIcon?: React.ReactNode;
  rightIcon?: React.ReactNode;
  error?: string;
  label?: string;
  hint?: string;
}

export const Input = forwardRef<HTMLInputElement, InputProps>(
  ({ className, variant, size, state, leftIcon, rightIcon, error, label, hint, type, ...props }, ref) => {
    const hasError = Boolean(error);
    const currentState = hasError ? 'error' : state;

    return (
      <div className="w-full">
        {label && (
          <label className="block text-sm font-medium text-white mb-2">
            {label}
          </label>
        )}
        
        <div className="relative">
          {leftIcon && (
            <div className="absolute inset-y-0 left-0 flex items-center pl-3 pointer-events-none">
              <span className="h-4 w-4 text-secondary-400">{leftIcon}</span>
            </div>
          )}
          
          <input
            type={type}
            className={cn(
              inputVariants({ variant, size, state: currentState }),
              leftIcon && 'pl-10',
              rightIcon && 'pr-10',
              className
            )}
            ref={ref}
            {...props}
          />
          
          {rightIcon && (
            <div className="absolute inset-y-0 right-0 flex items-center pr-3 pointer-events-none">
              <span className="h-4 w-4 text-secondary-400">{rightIcon}</span>
            </div>
          )}
        </div>
        
        {(error || hint) && (
          <div className="mt-1 text-sm">
            {error ? (
              <span className="text-danger-400">{error}</span>
            ) : (
              <span className="text-secondary-400">{hint}</span>
            )}
          </div>
        )}
      </div>
    );
  }
);

Input.displayName = 'Input';

// Specialized input components
export const SearchInput = forwardRef<HTMLInputElement, Omit<InputProps, 'leftIcon' | 'type'>>(
  (props, ref) => {
    return (
      <Input
        {...props}
        ref={ref}
        type="search"
        leftIcon={
          <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
            <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="m21 21-6-6m2-5a7 7 0 11-14 0 7 7 0 0114 0z" />
          </svg>
        }
        placeholder="Search..."
      />
    );
  }
);

SearchInput.displayName = 'SearchInput';

export const NumberInput = forwardRef<HTMLInputElement, Omit<InputProps, 'type'>>(
  ({ min = 0, step = "any", ...props }, ref) => {
    return (
      <Input
        {...props}
        ref={ref}
        type="number"
        min={min}
        step={step}
      />
    );
  }
);

NumberInput.displayName = 'NumberInput';
