import ifj26.*

def fib(n: Int): Int =
  if n < 2 then
    return n
  else
    return fib(n - 1) + fib(n - 2)

@main
def main(): Unit =
  print(toStr(fib(10)))
