import ifj26.*

def fact(n: Int)(acc: Int): Int =
  if n <= 1 then
    return acc
  else
    return fact(n - 1)(acc * n)

@main def main(): Unit =
  val x = fact(10)(1)
  print(x)
  print("\n")
