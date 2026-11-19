import ifj26.*

def sumTo(n: Int): Int =
  var s = 0
  for i <- 1 to n do
    s = s + i
  s

def prodTo(n: Int): Int =
  var p = 1
  for i <- 1 to n do
    p = p * i
  p

@main
def main(): Unit =
  val a = sumTo(4)
  val b = prodTo(4)
  print(a)
  print(",")
  print(b)
