import ifj26.*

def sum(n: Int): Int =
  var s = 0
  for k <- 1 to n do s = s + k
  s

@main def main(): Unit =
  val a = sum(100)
  print(a)
  print("\n")
