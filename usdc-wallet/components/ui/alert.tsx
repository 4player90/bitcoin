import * as React from 'react'
import { cn } from '@/lib/utils'

const Alert = ({ className, ...props }: React.HTMLAttributes<HTMLDivElement>) => (
  <div role="alert" className={cn('w-full rounded-lg border border-gray-200 bg-gray-50 px-4 py-3 text-sm', className)} {...props} />
)
const AlertDescription = ({ className, ...props }: React.HTMLAttributes<HTMLDivElement>) => (
  <div className={cn('text-sm whitespace-pre-line', className)} {...props} />
)

export { Alert, AlertDescription }
