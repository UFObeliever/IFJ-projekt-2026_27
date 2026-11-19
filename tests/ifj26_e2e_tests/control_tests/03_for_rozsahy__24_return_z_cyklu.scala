import ifj26.*

def firstOver(n: Int): Int =
  for i <- 1 to 100 do
    if i * i > n then
      return i
  return 0

@main
def main(): Unit =
  val a = firstOver(50)
  val b = firstOver(100000)
  print(a)
  print(",")
  print(b)
