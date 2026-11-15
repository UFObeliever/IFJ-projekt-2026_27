import ifj26.*

def firstAbove(n: Int): Int =
  var i = 0
  while i < 1000 do
    val sq = i * i
    if sq > n then
      return i
    else
      i = i + 1
  return -1

@main def main(): Unit =
  val a = firstAbove(50)
  val b = firstAbove(2000000)
  print(a)
  print("\n")
  print(b)
  print("\n")
