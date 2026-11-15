import ifj26.*

def depthSum(n: Int): Int =
  if n == 0 then
    return 0
  else
    val m = n - 1
    val local = n * 10
    val rest = depthSum(m)
    return local + rest

@main def main(): Unit =
  val a = depthSum(4)
  val b = depthSum(100)
  print(a)
  print("\n")
  print(b)
  print("\n")
