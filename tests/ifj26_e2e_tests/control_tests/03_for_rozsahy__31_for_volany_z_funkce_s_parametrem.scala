import ifj26.*

def sumTo(n: Int): Int =
  var s = 0
  for i <- 1 to n do
    s = s + i
  s

@main
def main(): Unit =
  val r = sumTo(10)
  print(r)
