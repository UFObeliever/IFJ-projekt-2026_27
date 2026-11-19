import ifj26.*

def f(n: Int)(acc: Int): Int =
  if n == 0 then
    return acc
  else
    return f(n - 1)(acc * n)

@main def main(): Unit =
  val x = f(5)(1)
  print(x)
