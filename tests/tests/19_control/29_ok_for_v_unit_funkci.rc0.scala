import ifj26.*

def f(n: Int): Unit =
  for i <- 1 to n do print(i)

@main def main(): Unit =
  f(2)
