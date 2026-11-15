import ifj26.*

def isEven(n: Int): Int =
  if n == 0 then
    return 1
  else
    val m = n - 1
    val r = isOdd(m)
    return r

def isOdd(n: Int): Int =
  if n == 0 then
    return 0
  else
    val m = n - 1
    val r = isEven(m)
    return r

@main def main(): Unit =
  val a = isEven(10)
  val b = isOdd(7)
  val c = isEven(7)
  print(a)
  print("\n")
  print(b)
  print("\n")
  print(c)
  print("\n")
