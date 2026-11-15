import ifj26.*

def sumTo(n: Int): Int =
  if n == 0 then
    return 0
  else
    val m = n - 1
    val r = sumTo(m)
    return n + r

@main def main(): Unit =
  val s = sumTo(2000)
  print(s)
  print("\n")
