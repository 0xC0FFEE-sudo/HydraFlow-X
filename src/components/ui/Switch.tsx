'use client';

import { forwardRef } from 'react';
import * as SwitchPrimitives from '@radix-ui/react-switch';
import { cva, type VariantProps } from 'class-variance-authority';
import { cn } from '@/lib/utils';

const switchVariants = cva(
  'peer inline-flex shrink-0 cursor-pointer items-center border-2 border-transparent transition-colors focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-primary-500 focus-visible:ring-offset-2 focus-visible:ring-offset-secondary-900 disabled:cursor-not-allowed disabled:opacity-50',
  {
    variants: {
      size: {
        sm: 'h-4 w-7 rounded-full',
        default: 'h-5 w-9 rounded-full', 
        lg: 'h-6 w-11 rounded-full',
      },
      variant: {
        default: 'data-[state=checked]:bg-primary-600 data-[state=unchecked]:bg-secondary-600',
        success: 'data-[state=checked]:bg-success-600 data-[state=unchecked]:bg-secondary-600',
        warning: 'data-[state=checked]:bg-warning-600 data-[state=unchecked]:bg-secondary-600',
        danger: 'data-[state=checked]:bg-danger-600 data-[state=unchecked]:bg-secondary-600',
        glass: 'data-[state=checked]:bg-primary-500/80 data-[state=unchecked]:bg-white/20 backdrop-blur-md border border-white/20',
      },
      glow: {
        none: '',
        subtle: 'data-[state=checked]:shadow-glow-sm',
        medium: 'data-[state=checked]:shadow-glow',
      },
    },
    defaultVariants: {
      size: 'default',
      variant: 'default',
      glow: 'none',
    },
  }
);

const thumbVariants = cva(
  'pointer-events-none block rounded-full bg-white shadow-lg ring-0 transition-transform',
  {
    variants: {
      size: {
        sm: 'h-3 w-3 data-[state=checked]:translate-x-3 data-[state=unchecked]:translate-x-0',
        default: 'h-4 w-4 data-[state=checked]:translate-x-4 data-[state=unchecked]:translate-x-0',
        lg: 'h-5 w-5 data-[state=checked]:translate-x-5 data-[state=unchecked]:translate-x-0',
      },
    },
    defaultVariants: {
      size: 'default',
    },
  }
);

export interface SwitchProps
  extends React.ComponentPropsWithoutRef<typeof SwitchPrimitives.Root>,
    VariantProps<typeof switchVariants> {
  label?: string;
  description?: string;
  error?: string;
}

export const Switch = forwardRef<
  React.ElementRef<typeof SwitchPrimitives.Root>,
  SwitchProps
>(({ className, size, variant, glow, label, description, error, ...props }, ref) => (
  <div className="flex items-center space-x-3">
    <SwitchPrimitives.Root
      className={cn(switchVariants({ size, variant, glow }), className)}
      {...props}
      ref={ref}
    >
      <SwitchPrimitives.Thumb className={cn(thumbVariants({ size }))} />
    </SwitchPrimitives.Root>
    
    {(label || description) && (
      <div className="flex flex-col">
        {label && (
          <label className="text-sm font-medium text-white cursor-pointer">
            {label}
          </label>
        )}
        {description && (
          <p className="text-xs text-secondary-400">
            {description}
          </p>
        )}
        {error && (
          <p className="text-xs text-danger-400 mt-0.5">
            {error}
          </p>
        )}
      </div>
    )}
  </div>
));

Switch.displayName = SwitchPrimitives.Root.displayName;

// Preset switch components for common use cases
export const ToggleSwitch = forwardRef<
  React.ElementRef<typeof SwitchPrimitives.Root>,
  Omit<SwitchProps, 'label'> & { 
    onLabel?: string; 
    offLabel?: string;
    showLabels?: boolean;
  }
>(({ onLabel = 'On', offLabel = 'Off', showLabels = false, ...props }, ref) => (
  <div className="flex items-center space-x-2">
    {showLabels && (
      <span className="text-sm text-secondary-400 min-w-[2rem]">
        {offLabel}
      </span>
    )}
    <Switch {...props} ref={ref} />
    {showLabels && (
      <span className="text-sm text-secondary-400 min-w-[2rem]">
        {onLabel}
      </span>
    )}
  </div>
));

ToggleSwitch.displayName = 'ToggleSwitch';

export const FeatureSwitch = ({
  feature,
  enabled,
  onToggle,
  disabled = false,
  ...props
}: {
  feature: {
    name: string;
    description: string;
    icon?: React.ReactNode;
  };
  enabled: boolean;
  onToggle: (enabled: boolean) => void;
  disabled?: boolean;
} & Omit<SwitchProps, 'label' | 'description' | 'checked' | 'onCheckedChange'>) => (
  <div className="flex items-start space-x-3 p-3 rounded-lg hover:bg-white/5 transition-colors">
    {feature.icon && (
      <div className="flex-shrink-0 mt-0.5 text-primary-400">
        {feature.icon}
      </div>
    )}
    <div className="flex-1 min-w-0">
      <div className="flex items-center justify-between">
        <div>
          <h4 className="text-sm font-medium text-white">
            {feature.name}
          </h4>
          <p className="text-xs text-secondary-400 mt-0.5">
            {feature.description}
          </p>
        </div>
        <Switch
          checked={enabled}
          onCheckedChange={onToggle}
          disabled={disabled}
          size="sm"
          {...props}
        />
      </div>
    </div>
  </div>
);
