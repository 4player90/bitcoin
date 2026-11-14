'use client'
import * as React from 'react'
import { cn } from '@/lib/utils'

const TabsContext = React.createContext<{ value: string; setValue: (v: string) => void }>({
  value: '',
  setValue: () => {},
})

function Tabs({ defaultValue, className, ...props }: React.HTMLAttributes<HTMLDivElement> & { defaultValue: string }) {
  const [value, setValue] = React.useState(defaultValue)
  return (
    <TabsContext.Provider value={{ value, setValue }}>
      <div className={className} {...props} />
    </TabsContext.Provider>
  )
}

const TabsList = ({ className, ...props }: React.HTMLAttributes<HTMLDivElement>) => (
  <div role="tablist" className={cn('inline-flex h-10 items-center rounded-lg bg-gray-100 p-1', className)} {...props} />
)

function TabsTrigger({
  value,
  className,
  disabled,
  ...props
}: React.ButtonHTMLAttributes<HTMLButtonElement> & { value: string }) {
  const ctx = React.useContext(TabsContext)
  const active = ctx.value === value
  return (
    <button
      role="tab"
      aria-selected={active}
      disabled={disabled}
      onClick={() => ctx.setValue(value)}
      className={cn(
        'rounded-md px-3 py-1.5 text-sm font-medium transition-colors disabled:opacity-50',
        active ? 'bg-white shadow' : 'text-gray-500',
        className
      )}
      {...props}
    />
  )
}

function TabsContent({ value, className, ...props }: React.HTMLAttributes<HTMLDivElement> & { value: string }) {
  const ctx = React.useContext(TabsContext)
  if (ctx.value !== value) return null
  return <div role="tabpanel" className={className} {...props} />
}

export { Tabs, TabsList, TabsTrigger, TabsContent }
