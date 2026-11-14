import * as React from 'react'
import { cn } from '@/lib/utils'

export type ButtonProps = React.ButtonHTMLAttributes<HTMLButtonElement>

const Button = React.forwardRef<HTMLButtonElement, ButtonProps>(
  ({ className, ...props }, ref) => (
    <button
      ref={ref}
      className={cn(
        'inline-flex h-10 items-center justify-center gap-2 rounded-md bg-black px-4 py-2 text-sm font-medium text-white shadow transition-colors hover:bg-black/80 disabled:pointer-events-none disabled:opacity-50',
        className
      )}
      {...props}
    />
  )
)
Button.displayName = 'Button'

export { Button }
