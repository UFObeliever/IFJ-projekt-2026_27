import ifj26.*

def f(n: Int): Int =
  val t = n + 1
  return t

def g(n: Int): Int =
  val t = n * 100
  val u = f(n)
  return t + u

@main def main(): Unit =
  val t = 7
  val r = g(t)
  print(r)
  print("\n")
  print(t)
  print("\n")
